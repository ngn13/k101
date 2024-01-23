# Zafiyetli Modülü İncelemek   
Daha önce hiç kernel modülü yazmadıysanız merak etmeyin, bu inceleme gayet basit ve açıklayıcı olacak.

Lab'deki `/root`daki `module` dizinine bakacak olursanız modül dosyalarını göreceksiniz:
```
module 
|- Makefile
|- vuln.c
```
Hadi sırası ile bu dosyaları inceleyelim.

### `Makefile`
`make` komutu ile derlemede otomasyon sağlamak adına dizinde bir adet Makefile bulunuyor.
Bu `Makefile`daki tek hedef olan ve modülümüzü derleyen `all` hedefine bakalım:
```
all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
```
Bu kural tekrar bir make komutu çağrıyor, bu sefer `-C` parametresi ile 
`/lib/modules/$(shell uname -r)/build/` altında. 

Bu dizin ismindeki shell komutu çalışınca tam yolumuz anlık kullandığımız kernelin source'una 
işaret edicektir. Bunu shell'inizde `cd /lib/module/$(uname -r)/build` çalıştırarak da görebilirsiniz.

Yani bu make komutu kernelin make dosyasındaki `modules` hedefini çağrıyor. Bu hedefe aynı zamanda
`M=$(PWD)` parametresi verilmiş, bu modül olarak şuan içinde bulunduğumuz dizindeki modül kaynak 
kodunun yani `vuln.c`'nin derlenmesini sağlıyor.

Bunun dışında hedef çıktıyı, `Makefile`'ın başındaki `obj-m := vuln.o` ile belirtiyoruz.

`all` hedefinin çalışmasını `make` komutunu çalıştırarak görebiliriz. Sonuç olarak birçok dosya 
dizini dolduracak ancak bizi alakadar eden tek önemli dosya: `vuln.ko`. 

### Kmod komutları
Kernel modülleri ile kolayca interaksiyonda bulunmak adına çoğu GNU/Linux dağtımında 
hazırda dağtılan popüler bir araç. Bu araç ile rahatça kernelde yüklü olan modülleri listeleyebilir,
yeni modüller yükleyebilir veya bir modülü kernelden çıkartabiliriz.

Arkaplanda bu araç `init_module` gibi linux syscall'larını çağırmakta.

Kmod ile yeni bir modül yüklemek adına:
```bash
insmod [modül dosyası]
```
Yüklü olan modülleri listemek adına:
```bash 
lsmod
```
Kmod bir modül kaldırmak adına:
```bash
rmmod [modül adı]
```
Az sonra, bizim derlenmiş modülümüz olan `vuln.ko` dosyasını kmod kernele yüklüyor olacağız. 
Fakat şimdilik modülün kaynak kodunu inceleyelim.

### `vuln.c`
Bu dosya modülün C kaynak kodunu içeriyor.
İlk olarak başta eklediğimiz header dosyaları var:
```c
#include <linux/module.h>       
#include <linux/kernel.h>       
#include <linux/proc_fs.h>      
#include <linux/uaccess.h>        
```
Bu header dosyaları kernel modüllerine özel. `module.h` her modülün zorunlu olarak eklemesi gereken
header. Bunun dışında diğer headerlar farklı yerlerde kullandığımız fonksiyonların tanımlarını içeriyor.

Ardından `#define` ile tanımlanan bazı sabit değerlerimiz var:
```c
#define PROCFS_MAX_SIZE         32
#define PROCFS_NAME             "vuln"
```
Bunların nerelerde kullanıldığını az sonra göreceğiz.

Hemen aşağıda modülün lisansını belirtiyoruz, bu her modül için zorunlu, diğer türlü modül 
derlenmiyecektir.
```c
MODULE_LICENSE("GPL");
```

Ardından bazı statik global değişkenleri tanımları görüyoruz:
```c
static struct proc_dir_entry *proc_file;
static char procfs_buffer[PROCFS_MAX_SIZE];
static unsigned long procfs_buffer_size = 0;
```
Bildiğiniz gibi linux'da herşey bir dosya. Bu kernel modülü user-space ile iletişime geçmek için 
bir `/proc` dosyası oluşturuyor. İlk değişken olan `proc_file`, daha sonra oluşturulacak 
bu `proc_dir_entry` objesinin adresine işaret edicek, ikinci olan değişken `procfs_buffer`, yukarda 
tanımladığımız `PROCFS_MAX_SIZE` kadar karakter tutabilen küçük bir buffer. 
Son olarak `procfs_buffer_size` geçici olarak buffer büyüklüğünü tutmak için 
kullandığımız bir değişken. 

Şimdi modülümüzün ana fonksiyonu olan `init_module` fonksiyonuna bakalım. Bu modül ilk yüklendiğinde 
kernel tarafından çağrılan bir fonksiyon. User-space programlarındaki `main` fonksiyonu gibi 
düşünebilirsiniz:
```c
int init_module(){
  proc_file = proc_create(PROCFS_NAME, 0666, NULL, &fops);
  memset(procfs_buffer, 'A', PROCFS_MAX_SIZE);

  if (proc_file == NULL) {
    remove_proc_entry(PROCFS_NAME, NULL);
    printk(KERN_ALERT "[vuln] Cannot create /proc/%s\n", PROCFS_NAME);
    return -ENOMEM;
  }

  printk(KERN_INFO "[vuln] /proc/%s created\n", PROCFS_NAME);
  return 0;
}
```
İlk olarak `proc_create` fonksiyonu ile `PROCFS_NAME` ismine, `0666` dosya izinlerine (`-rw-rw-rw-`) ve 
`fops` isimli dosya operasyonlarına sahip olan bir `/proc` dosyası oluşturuyoruz.

Bu `fops` dosya operasyonlarına daha yakından bakalım:
```c
static struct proc_ops fops = {
  .proc_read = procfile_read,
  .proc_write = procfile_write,
};
```
Bu dosyaya birşeyler yazılınca veya dosyadan birşeyler okununca çağrılcak olan fonksiyonları 
belirtmek için kullandığımız dosya operasyonları tutuğumuz bir `proc_ops` `struct`ı. Bu fonksiyonları 
daha sonra daha yakından inceleyeceğiz.

`proc_create` aynı zamanda bir `NULL` argümanı alıyor, bu oluşturulacak `/proc` dosyasının ebeveynini
belirtmek için kullandığımız bir argüman, ancak bu dosyanın herhangi bir ebeveyni olmayacağından ve 
argümanın tipi bir pointer olduğundan doğrudan `NULL` geçiyoruz.

`proc_create` fonksiyonunu çağrıp yeni `/proc` dosyamızı oluşturduktan sonra `procfs_buffer`ımızı 
tamamen `A` karakteri ile dolduruyoruz. 

Hemen arkasından `proc_create` fonksiyonunun sonucu döndürülen `proc_file` `NULL`mu diye kontrol 
ediyoruz, durum bu ise `proc_create` başarısız olmuş demektir. Durum bu ise dosyayı kaldırıp 
`printk` ile bir mesaj bırakıp `ENOMEM` hata kodu ile dönüyoruz.

`printk` fonksiyonu user-space programlarındaki `printf` fonksiyonuna oldukça benzer. Bu fonksiyonun 
başına önce çıktının türünü bildiren bir makro ekliyoruz, bu örnekte `KERN_INFO`. Formatlaması 
`printf` fonksiyonu ile tamamen aynı, ancak çıktı klasik user-land programlarında olduğu gibi
stdout'a yani ekrana düşmek yerine kernel kayıtlarına düşüyor. Bu kayıtları istediğimiz zaman `dmesg`
komutu ile okuyabiliriz. 

Eğer bu kayıtların sürekli olarak güncellenerek terminalize basılmasını istiyorsanız `dmesg -wH` 
ile kayıtları `ctrl+c` basana kadar izleyebilirsiniz, eğer kernel kayıtlarını temizlemek istiyorsanız 
`dmesg -C` komutunu kullanabilirsiniz.

Herneyse, eğer `proc_file` başarı ile oluşturulmuş ise dosyanın oluşturulduğu hakkında bir mesaj 
bıraktıktan sonra fonksiyonumuzdan 0 kodu, yani sorunsuz olarak dönüyoruz.

Şimdi gelin `/proc` dosyasından okuma yapılınca çağrılan `procfile_read` fonksiyonuna bakalım:
```c
static ssize_t procfile_read(struct file *file, char *buffer, size_t count, loff_t *offset){
  char local[PROCFS_MAX_SIZE];
  memcpy(local, procfs_buffer, PROCFS_MAX_SIZE);
  printk(KERN_INFO "[vuln] Reading %d bytes\n", count);
  memcpy(buffer, local, count);
  return count;
}
```
Bu fonksiyonun ilk parametresi `/proc` dosyamıza giden bir pointer. `buffer` ise user-space'de olan 
ve okunulan verinin yazılacağı karakter listesi. `count` ise okunacak karakter sayısı.

Bu fonksiyon aslında modülümüzdeki ilk zafiyeti içeriyor, arbitary memory read.

İlk olarak fonksiyonda lokal bir buffer oluşturuyoruz. `procfs_buffer`ımız ile aynı büyüklüğe sahip 
olan bu local buffera daha sonra `procfs_buffer`'ın içeriğini kopyalıyoruz.

Sonrasında bu lokal bufferdan `count` yani okuanan karakter kadar karakteri user-space buffer'ına 
kaydediyoruz.

İşte burda zafiyet meydana geliyor. **Eğer okunan karakter sayısı** `PROCFS_MAX_SIZE`**'dan büyükse** 
`memcpy` **fonksiyonu stack üzerinden** `buffer`**a fazladan bellek kopyalıyacaktır.** Bu durum 
arbitary memory read zafiyetine sebebiyet veriyor.

Ardından `count` değişkenini döndürüyoruz. Bu doğrudan user-space'e dönüyor, okuma işlemi sonucu 
okunan karakter sayısının döndürülmesi beklendiğinden bunu yapmazsak user-space programlar 
okuma işleminin başarısız olduğunu düşünebilir.

Şimdi ise `/proc` dosyamıza birşeyler yazılınca çağrılan `procfile_write` fonksiyonuna bakalım:
```c
static ssize_t procfile_write(struct file *file, const char *buffer, size_t count, loff_t *offset){
  char local[8];
  procfs_buffer_size = count;

  if (copy_from_user(procfs_buffer, buffer, count))
    return -EFAULT;

  memcpy(local, procfs_buffer, procfs_buffer_size);
  printk(KERN_INFO "[vuln] Copied to buffer: %s", local);
  return procfs_buffer_size;
}
```
Parametreler aynı, diğer fonksiyondaki gibi local bir buffer'ımız var, ancak bu buffer okuma 
fonksiyonunda olduğu gibi, `PROCFS_MAX_SIZE` boyutunda değil, sadece 8 byte. 

Sanırım bunun nereye gittiğini görebiliyorsunuz. `copy_from_user` ile 
user-space'de olan `buffer`'ından `procfs_buffer`'ına yazılan miktarda (`count`) içerik kopyalıyoruz.
Bu kodun zafiyetli olduğu yanılgısına kapılabilirsiniz ancak `copy_from_user` fonksiyonu 
`buffer`dan kopyalanan miktarın `procfs_buffer`dan büyük olmadığını kontrol ediyor, eğer büyükse 
kopyalama gerçekleşmiyor, bu durumda zaten dönüş değerinden kaynaklı `if` fonksiyonuna dönüyoruz.

İkinci zafiyetimiz sıradaki `memcpy` fonksiyonunda ortaya çıkıyor, burda 8 byte büyüklüğündeki 
lokal buffer'a `procfs_buffer`dan yazılan miktarda veri kopyalıyoruz. `procfs_buffer`ın büyüklüğü 
`PROCFS_MAX_SIZE` kadar, yani 32 byte. `count` değişkeni de maksimum 32 byte, ancak kopyalama 
işlemini yaptığımız hedef lokal buffer'ımız 8 byte. Bundan kaynaklı burda lokal buffer taşarak 
buffer overflow zafiyetine sebebiyet veriyor. 

`memcpy` den sonra lokal bufferı kernel kayıtlarına basıp, yazılan karakter sayısını döndürüyoruz.

Son olarak `cleanup_module` fonksiyonumuz var. Bu fonksiyon modülümüz kernelden çıkartılınca 
çağrılıyor:
```c
void cleanup_module(){
  remove_proc_entry(PROCFS_NAME, NULL);
  printk(KERN_INFO "[vuln] /proc/%s removed\n", PROCFS_NAME);
}
```
Sadece `remove_proc_entry`e `/proc` dosya adımızı veriyoruz, ikinci parametre yine ebeveyni belirtmek 
için, dosyamızın ebeveyni olmadığından yine bunu `NULL` bırakabiliriz. 

Sonrasında yine kayıtlara küçük bir mesaj bırakıyoruz. 

Aslında bir fonksiyonumuz daha var:
```c
void cant_get_here(void){
  printk(KERN_INFO "[vuln] How did we get here?\n");
}
```
Bu fonksiyonu ilk exploitimizi yazarken kullanacağız, sadece kernel kayıtlarına bir mesaj bırakıyor. Bunun dışında bu fonksiyon herhangi bir yerde çağrılmıyor ya da kullanılmıyor.  

Modülümüzü analiz ettik ve iki farklı zafiyet bulduk: arbitary memory read ve buffer overflow.
Artık modülümüzü kernele yükleyebiliriz ve exploitimizi yazma aşamasına geçebiliriz: `insmod vuln.ko`
Lütfen `dmesg` çıktısını kontrol ederek modülün başarı ile yüklendiğine emin olun. Ayrıca 
`ls -la /proc/vuln` çalıştırarak `/proc` dosyasının varlığını doğrulayın.

---
[Önceki](setup.md) | [Sonraki](first.md)
