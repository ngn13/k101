# Kernel Nedir?
Gündelik bilgisayar kullanımında, kullanıcıların etkilişime geçtiği, işletim sistemini oluşturan parçalar,
doğrudan donanım ile haberleşmez. İşlemcinin üzerinde çalıştığı yazılıma sunduğu farklı ayrıcalık (privilege) seviyeleri
bulunmaktadır.

Kullanıcının etkileşime geçtiği programlar, çoğunuzun da bildiğini tahmin ettiğim gibi bu ayrıcalıklar seviyesinden en
düşüğüne sahiptir. Bu programların donanım ile haberleşmek için bir köprü kullanması gerekir. Bu köprü kernel tarafından sağlanır.

Kernel, bir işletim sistemindeki en ayrıcalıklı programdır. Doğrudan donanım ile haberleşir ve kendinden daha az ayrıcalığa sahip programlara
bir köprü sunmanın yanı sıra, bu programları yönetir ve birbirleri ile beraber sağlıklı bir şekilde çalışmalarını sağlar. Kernel aynı zamanda
daha üst bir ayrıcalık seviyesinde çalıştığından, kullanıcı program'ları kernel'in bellek gibi farklı kaynaklarına erişmemez. Kernel ise
diğer tüm programların tüm kaynaklarına erişmek ile yetinmez, tüm kaynakları kontrol eder.

Bu sebepten ötürü, oldukça popüler bir saldırı noktasıdır. Kullanıcı aryıcalık düzeyinde çalışan bir program olarak, kernel'de olan bir zaafiyeti
kötüye kullanabilirseniz, tüm sisteme erişiminiz olur. Sizden daha yetkili bir kullanıcıya (örneğin root) erişim sağlayabilirsiniz, doğrudan donanım
ile haberleşerek donanım üzerinde olan zaafiyetleri kötüye kullanmaya başlayabilirsiniz vs.

Ve yine bu sebepten ötürü, özellikle bu rehberde hedeflediğimiz Linux kernel'i birçok farklı bellek korumasına sahip olmak ile beraber,
gece ve gündüz yüzlerce hatta binlerce güvenlik uzmanı tarafından incelenmektedir. Yani kernel'i kırması imkansız mı? Cevabı evet olsa
bu rehberi yazıyor olmazdım.

### Farklı Baskı Noktaları
Kernel aktif olarak yüz binlerce geliştirici tarafından geliştirildiği için, mainline kernel tree'sinde farklı aralıklarla ciddi zafiyetlere
rastlanabiliyor. Fakat bunun dışında kernel'in özgür ve açık kaynaklı olduğunu unutmamanız lazım. Linux kernel'i modüler bir yapıya sahip olduğundan
farklı organizasyonlar kendi kişisel kullanımları için kendi kernel modüllerini yazabilir. Bu tree dışı (out-of-tree, OOT) modüler, kendi başlarına
bir güvenlik problemi de oluşturabilir.

Bu sebepten ötürü, kernel güvenliğini öğrenmek kesinlikle zaman kaybı değildir. Bunun yanı sıra, kernel güvenliğini öğrenirken birçok yeni
şey de keşefedebilirsiniz.

# Kernel Mode vs. User Mode
User mode (userland, user-space), kullanıcı programlarının çalıştığı ayrıcalık seviyesine verilen isimdir. Öte yandan kernel mode (kernel-space), kernel'in
çalıştığı ayrıcalık seviyesine verilen isimdir. Bunlardan ring 3 (user mode) ve ring 0 (kernel mode) olarak bahsedildiğini de duymuş olabilirsiniz.
Bu "ring" isimlendirmesi, işlemci dışında, tüm donanım bileşenlerinin farklı ayrıcalık seviyeleri arasında sunduğu korumaları kapsar. Evet, bu "ring"ler
ya da "mode"lar sadece yazılımsal şeyler değil. Donanımsal olarak çalışan, fakat yazılımsal olarak kontrol edilen yapılar.

Peki bu ayrıcalık kontrolü tam olarak nasıl sağlanıyor? `x86_64`de, bu genel olarak [segment selector](https://wiki.osdev.org/Segment_Selector)
register'ları aracılığı ile sağlanıyor. 32 bit bir arkaplanı olanlar, bu registerları tanıyor olabilir, ama basitçe bu registerlar farklı bellek
alanları hakkında bilgi tutar. Örneğin spesifik olarak `cs`, code segment register'ı, işlemcinin şuan çalıştırdığı kodun bulunduğu bellek alanının
özelliklerini tanımlar. Bu register işlemciye şuan çalıştığı ring'i belirtmek için kullanılabilir. Bunun dışında farklı ring'lerin bellek erişimi,
[sayfalandırama](https://wiki.osdev.org/Paging) tarafından kontrol edilir. Sayfalandırma, basitçe hangi bellek adresinin, fiziksel bellekte nereye işaret
ettiğini, ve hangi özelliklere sahip olduğunu belirtilen bir tablo ile yapılır. Bu özellikleri belirtilen her bir bellek adresi bir sayfadır. Sayfaların
özellikleri arasında erişilebildikleri ring de bulunur.

Bu ve benzeri yapılar sayesinde, yazılım, işlemciye çalıştığı ring'i belirtebilir. Kernel doğrudan tüm bunları kontrol ettiğinden kernel'den user mode'a
geçmek genel olarak basittir. Öte yandan, user mode'da iken kernel mode'da geçmek biraz daha zor bir işlemdir. Bunun sebebi user mode'da iken, ring'i kontrol
eden tüm bu değişkenlere, güvenlik sebebi ile kolayca erişmemizdir. Örneğin kernel mode'da iken, sayfa tablosunu kontrol edip, modifiye edip, istediğimiz zaman
farklı sayfa tablolarını işlemciye yükleyerek farklı bellek alanların ring'lerini kontrol etmemiz mümkün iken, user mode'da bu tabloya erişmimiz yoktur. Benzer
bir şekilde, bir kaç instruction ile kernel mode'da `cs` ve diğer önemli segment register'larını değiştirmemiz mümkün iken, user mode'da, segment register'larına
anlık ayrıcalık seviyemizden (current privilege level, CPL) daha yüksek bir ayrıcalık seviyesine sahip bir segment selector'ü yüklememiz mümkün değildir.

Zaten köprü benzetmesinin ana açıklamarından biri de bu. User'ın tüm donanıma erişimi yok, çünkü bu erişime ihtiyacı yok. Kernel donanımın kullanımını, kullanıcı programı
için basitleştirir. Siz bir kernel üzerine program yazarken, size sunduğu bu arayüzü kullanırsınız. Bu sebepten ötürü diskten bir dosya okurken, dosya sistemini okuyup
anlamak ile uğraşmanız gerekmez, tüm o karmaşık işleri kernel sizin için yapar. Ya da bir ağ bağlantısdan size bir TCP mesajı iletirken, ağ sürücüsü ile haberleşmenize,
TCP ve IP paketlerini bit bit inşa etmenize gerek kalmaz, çünkü kernel bu sizin için yapar.

Kernel'in sunduğu bu arayüz, user mode'dan kernel'e geçiş yapmanın tek yoludur. Bu arayüz, sistem çağrılarıdır.

### Sistem Çağrılarını Anlamak
Sistem çağrılarını, istediğiniz zaman elinizin altında olan, kernel tarafından yönetilen bir dizi fonksiyon gibi düşünebilirsiniz. Linux'da, 64 bit'de bu çağrılar,
`syscall` instruction'ı aracılığı ile sağlanır. Bu AMD'nin özel olarak 64 bit mimarsi için geliştirdiği, Intel'in `sysenter` instruction'ına banzer olarak çalışan
bir instruction'dır. Öte yandan, `sysenter`dan farklı olarak bu instruction, [iki farklı işlemci modelinde de çalışır](https://wiki.osdev.org/SYSENTER#Compatibility_across_Intel_and_AMD).

Bu instruction, kernel tarafından gerekli ayarlar yapılınca, user mode'dan çağrıldığında, user mode'dan kernel'e hızlı bir geçiş sağlar. Bu geçişte, user mode programı
hiçbir şeyi kontrol etmez, tam olarak nereye geçiş yapılacağı doğrudan kernel tarafından kontrol edilir. Ve bu geçiş esnasında regitserlar ve stack korunduğundan,
kernel, user mode'un anlık durumunu, yapmak istediği eylemi anlamak için kullanır.

Ama yanlış anlamayın, bu, sistem çağrılarını çağırmanın tek yöntemi değil. `x86_64` mimarisinde bunu yapmanın farklı yolları var, eskiden özellikle 32 bit sistemlerde
yaygın olan yöntem, özel bir yazılım interrupt'ı çağırmaktan geçiyordu. Kernel bu interrupt'ın sistem çağrısı interrupt'u olduğunu bildiğinden, interrupt'ı sistem çağrılarına
ait fonksiyonları kullanmak için kullanıyordu. Klasik UNIX'de, ve Linux'da kullanılan bu interrupt sayısı 128 (`0x80`) idi. Bunun dışında
[farklı yöntemler de mevcut](https://wiki.osdev.org/System_Calls#Possible_methods_to_make_a_System_Call).

Her neyse devam edelim. Linux'da, diğer UNIX-gibi sistemler gibi bir dizi sistem çağrısı bulunur. 64 bit için bu çağrıların uzun bir listesine
[bu siteden](https://x64.syscall.sh/) erişebilirsiniz. Her çağrı klasik C çağrı yönetimini kullanarak parametreler kabul eder, ve hangi çağrının kullanımak
istendiği, `rax` regitserı ile belirtilir.

Örneğin, `42` hata kodu ile programı sonlandırmak için, user mode'dan `exit()` sistem çağrısını çağırabiliriz:
```asm
main:
    mov %rax, $60
    mov %rdi, $42
    syscall
```
`rax` burada `exit()` sistem çağrısının numarası olan 60 değerini tutar. `rdi` ise, klasik C çağrı yönetimi gereği ilk parametreyi tutar. `exit()` sistem
çağrısı için bu parametre hata kodudur.

`syscall` instruction'ı çalıştırıldığı zaman, işlemci kernel'in belirtiği sistem çağrılarını yöneten adrese atlıyacaktır. Bu şekilde kernel mode'a geçmiş oluruz.
Bu noktadan sonra kernel, `rax`i kontrol ederek hangi sistem çağrısını çağırmak istediğimize bakacaktır. Ardından bu sistem çağrısı için gerekli kernel fonksiyonunu çağrıp
istediğimiz işlemi yerine getirecektir. Ve klasik C çağrı yönetiminde olduğu gibi, programın dönüş değeri `rax` regitserına yazılacaktır. Kernel sistem çağrısını tamamladığında,
en son `sysret` instruction'ı ile user mode'da kaldığı yere geri dönecektir.

Bu aslında GNU C kütüphanesinin aşağıdaki çağrı ile arkaplanda yaptığı işlem ile aynıdır:
```c
int main(){
    exit(42);
}
```

### Zafiyetlere Erişmek
Linux'da herşeyin bir dosya olduğunu biliyoruz. Bu sistem çağrılarını kullanarak zaafiyetli bir karakter veya cihaz sürücüsüne erişebiliriz. Eğer bu sürücünün implementasyonu
kendisinden beklenen sistem çağrılarını doğru şekilde implemente etmiyorsa, bu bir zaafiyete sebebiyet verebilir, ve basit dosya işlemi çağrıları ile bu zaafiyeti kötüye
kullanabilirsiniz.

Çoğu CTF challenge'ında bunun gibi bir durum söz konusudur, genelde size zaafiyetli bir modül verilir. Bu modül bir cihaz, block sürücüsü, procfs girdisi vs. implement eder,
ve de siz bu zaafiyeti kötüye kullanarak bir çeşit bellek sorunana, race condition'a vs sebebiyet verip, root almaya çalışırsınız. Bunu incleyeceğimiz örnekler de göreceğiz.

Tabiki bu bir zaafiyete giden tek bir yol değil, herhangi bir hatalı implementasyon, doğrudan bir zafiyete gidebilir. Örneğin diyelim bir kernel'in farklı görevlerin
ne kadar çalışacağını planlamak ile görevli scheduler'ı, görevlerin yeniden isimlendirmelerini doğru şekilde gerçekleştirmiyor ve bir bellek sorununa sebebiyet veriyor,
bunu `prctl()` çağrısı ile bir göreve özel olarak tasarlanmış bir ad verip, kötüye kullanabilirsiniz.

Ya da diyelim ki `ptrace()` çağrısı, kendisini çağıran user mode programların, kötüye kullanılabilecek işlemci özelliklerini kontrol eden FLAGS registerını
[istediği gibi değiştirmesine izin veriyor](https://www.youtube.com/watch?v=1hpqiWKFGQs) olabilir, bu durumda bir `ptrace()` çağrısı ile user mode programlarına
doğrudan donanım erişimi verip, bunu kötüye kullanabilirsiniz.

Anlayacağınız birçok farklı saldırı vektörü söz konusu, fakat kernel mode'a geçip bu vekötere erişmek, genel olarak sistem çağrıları aracılığı ile gerçekleştirilir.

### Kernel Bellek Korumaları
User mode'daki programların RELRO, ASLR ya da stack çerezleri gibi, bazıları kernel tarafından sağlanan bellek korumalarına sahip olduğunu biliyor olabilirsiniz. Kernel'de de
benzeri korumalar mevcut, ve yeri geldiğinde bu korumlardan bahsediyor olacağız. Fakat hızlıca temel korumaları bir özet geçelim.

#### KASLR (Kernel Address Space Layout Randomization)
Normal şartlar altında, kernel belleğe her yüklendiğinde aynı adrese yüklenir. Bundan kaynaklı olarak
kernel ile beraber belleğe yüklenen fonksiyon ve benzeri semboller de aynı adrese yüklenir. Örneğin `prepare_kernel_cred` `0xffffffff81094a50` adresine yüklenmiş
olsun, bu adres sistem yeniden başlatıldığında değişmeyecektir. Bu durum herhangi bir kritik fonksiyonun adresini bilen bir saldırganın, kernelin akışını yönlendirmesini
sağlayan bir zafiyet kullanarak, kerneli bu fonksiyona yönlendirmesini sağlayabilir. Bunu önlemek adına KASLR, sistem her açıldığında kernelin yüklendiği adresi, dolayısı
ile de fonksiyon ve sembollerin adreslerini rastgele olarak değiştirir. Bu durum `0xffffffff81094a50` adresinde olan `prepare_kernel_cred` fonksiyonun sistem yeniden
başlayınca `0xffffffff421631e8` adresinde olmasını sağlayabilir. Bu şekilde saldırgan, kerneli istediği şekilde yönlendirmeden için önce fonksiyonun yeni adresini bulması gerekir.
KASLR kernel parametrelerine `nokaslr` eklenerek kapatılabilir.

Bunun implementasyonu kernel'in yüklendiği adresi rastgele hesaplanan bir offset ile aşağı/yukarı kaydırılmasından geçer. Bilindik bir sembolün adresi bellekden leak'lenebilirse,
bu offset hesaplanıp, KASLR kırılabilir.

#### Stack Cookies
User-space'de olan çerezlere benzer olarak, kernel-space bellekteki stack üzerine bir "çerez" yerleştirir. Eğer program akışında bu çerez herhangi bir
şekilde değiştirilirse, stack üzerindeki belleği değişmesine sebebiyet veren **buffer overflow** gibi bir zafiyet exploit edilmeye çalışılmıştır demektir. Bu durumda kernelde
exploit edilmeye çalışılan mümkün bir zafiyet olduğundan, `__stack_chk_fail()` fonksiyonu çağrılır, [kernel panikler](https://en.wikipedia.org/wiki/Kernel_panic) ve sistemin
çalışmasını durdurur.

Bunun imeplementasyonu, user-space impelemetasyonuna benzer şekilde, gerekli görülen yerlere, stack'e bu çerezleri push'layan, ve dönüşlerden
önce bu çerezleri kontrol eden instructionlar yerleştirmekten geçer.

Bunu kırmanın tek yolu, doğrudan dönüş adresini modifiye etmekten, ya da çerezi leak'lemekten geçer.

#### SMEP (Supervisor Mode Execution Prevention)
Bazı CPU implementasyonlarının bir özelliği olan SMEP, user-space belleğin "çalıştırılamaz" olmasını sağlar. Eğer saldırgan bir zafiyet aracılığı ile kernel'in
user-space'den bir kod çalıştırmasını sağlarsa, SMEP doğrudan CPU seviyesinde bir "trap"e yani hataya sebebiyet verecektir, bu hata daha sonra kernel'de karşılanır
ve çalıştırma durdurulur, kernel kaldığı yerden devam eder. **Bu durum kernel paniğine sebep olmaz ancak kernel kayıtlarına düşer (dmesg)** SMEP kernel parametrelerine
`nosmep` eklenerek kapatılabilir.

Bu implementasyon işlemci tarafından sağlanır, ve kernel'in bu korumayı aktifleştirmesi için gereken tek şey `cr4` regitser'ının 20. bitini 1 olarak ayarlamaktır.

5.1 versiyonundan daha eski kernel'lerde, bunu kırmak için basitçe `cr4`ün 20. biti 0'lanabilir. Bunun için `cr4`ü değiştiren bir kod parçasına ROP'lamak yeterli olacaktır.
Daha yeni kernel'de, `cr4` değiştiren kod parçaları bit pinning yöntemini kullandığından (20. bitin değişmediğinden emin olduğundan), tüm exploit ROP aracılığı ile yapılabilir,
ya da user-mode'a ROP aracılığı ile dönüş gerçekleştirilebilir.

#### SMAP (Supervisor Mode Execution Prevention)
Yine CPU implementasyonlarının bir özelliği olan SMAP, SMEP'e benzer olarak user-space belleğe etki eder, ancak bu user-space belleği "çalıştırılamaz" yapmak yerine "erişilemez"
yapar. Kernel eğer user-space'e erişmek isterse bir çeşit aç/kapa butonu gibi düşünebileceğiniz `STAC` ve `CLAC` instruction'larını kullanır, bu şekilde gerekli olunca user-space'e
olan erişimi açar ve ardından kapar. Eğer bir saldırgan vir zafiyet aracılığı ile kernel'e user-space'den bir bellek kopyalamaya çalışırsa SMAP doğrudan bir CPU seviyesinde "trap"e
sebebiyet verir. **Bu durum SMEP'e benzer olarak kernel paniğine sebep olmaz ancak kernel kayıtlarına düşer (dmesg)**. SMAP kernel parametrelerine `nosmap` eklenerek kapatılabilir.

Bunun implementasyonu, SMEP'e benzer bir şekilde işlemci tarafından sağlanır, ve kernel'in bu korumayı aktifleştirmesi için yapması gereken teş şey `cr4` register'ının 21. bitini
1 olarak ayarlamaktır.

Bunu kırmanın tek yolu user-mode'a hiçbir şekilde geri dönüş yapmamaktır. Tüm exploit ROP aracılığı ile sağlanmalıdır ve de exploit kernel belleğinde tutulmalıdır.

#### KPTI (Kernel Page-Table Isolation, PTI)
KASLR her ne kadar adresleri rastgele bir hale getirse de, meltdown ve spectre gibi, [modern işlemcilerdeki side-channel saldırıları](https://www.youtube.com/watch?v=x_R1DeZxGc0),
kernel-space'den bir adresin leaklenmesine ve KASLR'ın kırılmasına sebebiyet verebilir.

Bunu önlemek adına [KAISER (Kernel Address Isolation to have Side-channels Efficiently Removed)](https://gruss.cc/files/kaiser.pdf) ortaya atıldı. Bu korumanın temel fikri,
user-mode'a geçmeden önce sayfalandırma tablosunu, kernel'deki sadece minimal bir oranda belleği sayfalandıran başka bir tablo ile değiştirmek. Bu sayede, user-mode'da çalışan
bir program, bu side-channel saldırıları ile kernel'den kritik bir belleği, sayfalandırılmadığı için leaklemeyez.

KAISER'ın orijinal implementasyonunda bazı değişikler yapıldıktan sonra, KPTI olarak yeniden adlandırıldı. KPTI kernel parametrelerine `nopti` eklenerek kapatılabilir.

Bunun implementasyonu, kernel tarafından her işlem için iki ayrı sayfalandırma tablosu tutularak sağlanır. Kırmak içi basitçe, bu tablolar arasında geçiş yapmada kullanılan
`.Lpti_restore_regs_and_return_to_usermode` label'ı kullanılabilir. Tek yapmanız gereken bunu ROP'ununza eklemek olacaktır.

Artık kernel'i daha iyi anladığımıza göre, rehberi nasıl takip ediceğinizi açıkladıktan sonra ilk pratiğimiz ile işe başlayabiliriz. Merak etmeyin, burada anlatılan konseptleri,
yeri geldikçe genişletiyor olacağım.

# Bu rehberi takip etmek
Bu rehberde her bölümde farklı kernel CTF challenge'ları çözüyor ve detayları ile size bu challenge'ları açıklıyor olacağım. Beni takip edip sizde bu challenge'ları çözebilirsiniz.

İlk olarak, hala yapmadıysanız, bu repo'yu klonlamanız gerekecek. Ardından `src` dizini altında tüm challenge'lara, exploit'leri ile beraber erişebilirsiniz. Benim önerim kendi
önce rehberi okuyup sonra kendi exploitinizi yazmanız olacaktır. Ve evet her bölümde çözdüğümüz CTF challenge'ının kaynağını belirtiyor olacağım, merak etmeyin.

Challenge'ları takip ederken arada bir Linux kaynağını okumamız gerekebilir. Fakat her challenge farklı bir versiyon kullandığından, her versiyonun kaynak arşivini bulunduran
[elixir.bootlin.com](https://elixir.bootlin.com/linux) sitesi bu işi sizin için kolaylaştıracaktır. Farklı durumlarda kaynağı derlelememiz gerekebilir. Bu durumda kaynağı
[kernel.org](https://www.kernel.org/pub/linux/kernel/) HTTP suncusundan indirebilirsiniz ve derlemek için [bu dökümentasyonu](https://kernelnewbies.org/KernelBuild) okuyabilirsiniz
(tabiki de kurulum kısmına ihtiyacınız olmayacak).

Ayrıca versiyonlar arasında değişen önemli şeyler olduğunda, bunları size belirtiyor olacağım ve elimden geldiğince modern versiyonlarda çalışan yöntemler üzerinden ilerleyeceğim.

Hepsi bu kadar, hadi ilk challenge'ımıza geçelim.

---
[Önceki](README.md) | [Sonraki](kernel_adventures_2.md)
