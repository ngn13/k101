**NOT:** Kernel'in türkçesi olan "çekirdek" tam olarak anlamı karşılamadığından çeviride kullanmadım.

# Kernel'e genel bir bakış
Linux kernel'i oldukça geniş çaplı bir proje, her gün dünyanın her tarafından geliştiriciler 
kernel'e katkıda bulunmak için emailler, yamalar ve commit'ler gönderiliyor, bu dağınık yapıdan 
kurtulup gerçekten kernele girmeyi başaran birkaç parça kod, her ne kadar geliştiriciler tarafından 
incelense de her yıl farklı bir-iki kernel zafiyeti ortaya çıkıyor.

Ancak bu zafiyetleri anlamak için önce kernel'i anlamalıyız. 

### Kernel-space ve User-space
Email istemciniz, tarayıcınız, metin editörünüz, bütün bu programlar user-space'de çalışır.
Bu programlar işletim sisteminin/kernel'in kendisi için belirlediği kaynakları kullanabilir ve yine 
bu belirlenen kaynaklara erişebilir, örneğin user-space'de olan bir program kernel izin vermedikçe 
başka bir programın belleğini okuyamaz.

Öte yandan işletim sisteminin kernel'i (çekirdeği) ve kernel ile beraber çalışan driver'lar modüler vs. 
sistemdeki en yetkili programdır ve kernel-space'de çalışır, doğrudan donanım kaynaklarına erişir,
bunları diğer programların kullanabilmesi adına doğru şekilde yönetir, herhangi bir kısıtlama 
altında da bulunmaz. Doğrudan istediği belleği okuyabilir.

Kernel-space'den user-space'e geçiş yapılabilir. Yani kernel isterse user-space'de çalışan 
programların belleklerine erişebilir, progamları öldürebilir. Ancak user-space'den 
kernel-space'e geçiş yapılamaz. User-space'de çalışan bir program kesinlikle ama kesinlikle 
kernel-space'de yer alan bir belleğe erişemez, kernel'i öldüremez. 

Yani genel olarak bu böyle ancak şimdi biraz daha yakından bakacağımız kernel zafiyetleri 
tam olarak da bunu yapmamızı sağlıyor. Bu zafiyetler aracılğı ile user-space'de çalışarak 
kernel-space'e erişim sağlayabiliyoruz. Kernel-space'e eriştikten sonra gerisi sizin hayal 
gücünüze kalmış. İsterseniz root (en yetkili kullanıcı) olabilirsiniz, 
isterseniz sistemi çökertebilirsiniz, tüm sistem sizin elinizde.

Genel olarak kernel-space ve user-space mantığını anladığımıza göre, kernel'de bulunan bütün bu zafiyetler nasıl çalışıyor?

### Farklı çeşit kernel zafiyetleri 
Birçok farklı kernel zafiyeti mevcut ve hepsi her ne kadar farklı olsa da arkalarında yatan
temel konseptler var, günümüzdeki modern kernel zafiyetleri şu sınıflarda toplanabilir:

- **Integer Underflow/Overflow**: Bir sayı değişkeni maksimum değerinden daha büyük bir sayı alınca veya 
minimum değerinden daha küçük bir sayı alınca ortaya çıkar. Duruma bağlı olarak bu değişkenin başka 
bir rakama yuvarlanmasına neden olabilir, bu da farklı bellek saldırılarını mümkün kılar.

- **Arbitrary Memory Read**: Bellekten yapılan okuma işlemleri doğru şekilde kontrol edilmediğinde 
veya limitlenmediğinde ortaya çıkar. Bu tür bir zafiyet, istediğimiz gibi program belleğini okumamıza 
izin verir, bu durum bellekteki hassas bilgileri ortaya çıkartabilir ve farklı bellek saldırılarını 
mümkün kılar.

- **Arbitrary Memory Write**: Belleğe yapılan yapılan yazma işlemleri doğru şekilde kontrol 
edilmediğinde veya limitlenmediğinde ortaya çıkar. Bu tür bir zafiyet istediğimiz gibi program 
belleğine değerler yazmamıza izin verir. Bu durum belleği, dolayısı ile programın akışını 
değiştirmemizi sağlar.

- **Use-After-Free (UAF)**: Bellekten ayrılan herhangi bir alanın serbest bırakıldıktan sonra 
kullanılmasıdır. Bu durumda program serbest bırakılan belleğe erişmeye çalışır. Bu duruma farklı 
olarak farklı bellek saldırılarına kapı açar.

- **Buffer Overflow**: Herhangi birşey için bellekte açılan arabelleğin taşması durumunda ortaya çıkar.
Bu durumda taşmanın gerçekleştiği belleğin altında kalan belleğe erişim sağlanabilir, bu şekilde 
programın akışı değiştirilebilir.

- **Race-Condition**: Bir kaynağa iki veya daha fazla nesne aynı anda erişmeye çalıştığında ortaya çıkar.
Bu kaynağa doğru bir "yarış" durumu sağlar. Diğer türlerden farklı olarak bellek üzerine kurulu bir 
zafiyet çeşiti değil, ancak diğer bellek üzerine kurulu zafiyetlerden birine sebebiyet verebilir.

**ÖNEMLİ:** Bu zafiyet sınıfları kernel zafiyetlerine özgü değil, bu tarz zafiyetler genel user-space
programlarında da bulabileceğiniz şeyler.

Farketmiş olabileceğiniz gibi çoğu zafiyet türü bellek üzerine kurulu. Bundan kaynaklı olarak 
şimdi inceleyeceğimiz kernel korumalarının genel olarak bellek üzerine kurulu olduğunu göreceksiniz.

### Kernel korumaları
Kernel, kendisini ortaya çıkabilecek zafiyetlerden korumak adına farklı koruma yöntemleri/teknolojileri
içerir. Bunları bilmemiz zamanı gelince bypass edebilmemiz adına oldukça önemli:

- **KASLR (Kernel address space layout randomization)**: Normal şartlar altında, kernel belleğe her 
yüklendiğinde aynı adrese yüklenir. Bundan kaynaklı olarak kernel ile beraber belleğe yüklenen 
fonksiyon ve benzeri semboller de aynı adrese yüklenir. Örneğin `prepare_kernel_cred` `0xffffffff81094a50`
adresine yüklenmiş olsun, bu adres sistem yeniden başlatıldığında değişmeyecektir. Bu durum 
herhangi bir kritik fonksiyonun adresini bilen bir saldırganın, kernelin akışını yönlendirmesini
sağlayan bir zafiyet kullanarak, kerneli bu fonksiyona yönlendirmesini sağlayabilir. Bunu önlemek adına 
KASLR, sistem her açıldığında kernelin yüklendiği adresi, dolayısı ile de fonksiyon ve sembollerin 
adreslerini rastgele olarak değiştirir. Bu durum `0xffffffff81094a50` adresinde olan `prepare_kernel_cred`
fonksiyonun sistem yeniden başlayınca `0xffffffff421631e8` adresinde olmasını sağlayabilir. Bu şekilde 
saldırgan, kerneli istediği şekilde yönlendirmeden için önce fonksiyonun yeni adresini bulması gerekir.
KASLR kernel parametrelerine `nokaslr` eklenerek kapatılabilir.

- **Stack Cookies**: User-space'de olan çerezlere benzer olarak, kernel-space bellekteki stack 
üzerine bir "çerez" yerleştirir. Eğer program akışında bu çerez herhangi bir şekilde değiştirilirse,
stack üzerindeki belleği değişmesine sebebiyet veren **buffer overflow** gibi bir zafiyet 
exploit edilmeye çalışılmıştır demektir. Bu durumda kernelde exploit edilmeye çalışılan mümkün 
bir zafiyet olduğundan, kernel panikler (kernel panic, BSOD) ve sistemin çalışmasını durdurur.
Kernel paniklerinden az sonra daha detaylı bahsedeceğiz.

- **SMEP (Supervisor Mode Execution Prevention)**: Bazı CPU implementasyonlarının bir özelliği olan SMEP,
user-space belleğin "çalıştırılamaz" olmasını sağlar. Eğer saldırgan bir zafiyet aracılığı ile kernel'in 
user-space'den bir kod çalıştırmasını sağlarsa, SMEP doğrudan CPU seviyesinde bir "trap"e yani 
hataya sebebiyet verecektir, bu hata daha sonra kernel'de karşılanır ve çalıştırma durdurulur, kernel
kaldığı yerden devam eder. **Bu durum kernel paniğine sebep olmaz ancak kernel kayıtlarına düşer (dmesg)**
SMEP kernel parametrelerine `nosmep` eklenerek kapatılabilir.

- **SMAP (Supervisor Mode Execution Prevention)**: Yine CPU implementasyonlarının bir özelliği olan SMAP,
SMEP'e benzer olarak user-space belleğe etki eder, ancak bu user-space belleği "çalıştırılamaz" yapmak 
yerine "erişilemez" yapar. Kernel eğer user-space'e erişmek isterse bir çeşit aç/kapa butonu gibi 
düşünebileceğiniz `STAC` ve `CLAC` komutlarını kullanır, bu şekilde gerekli olunca user-space'e olan 
erişimi açar ve ardından kapar. Eğer bir saldırgan vir zafiyet aracılığı ile kernel'e user-space'den 
bir bellek kopyalamaya çalışırsa SMAP doğrudan bir CPU seviyesinde "trap"e sebebiyet verir. **Bu durum
SMEP'e benzer olarak kernel paniğine sebep olmaz ancak kernel kayıtlarına düşer (dmesg)**
SMAP kernel parametrelerine `nosmap` eklenerek kapatılabilir.

- **KAISER (Kernel page-table isolation)**: Meltdown gibi CPU'dan kaynaklı ortaya çıkan side-channel
zafiyetleri önlemek adına ortaya çıkmıştır. Bu koruma, kernel-space komutlarına giriş yapıldığında 
kernel'in user-space belleğini tamamı ile kopyalamasını sağlar, kernel-space'de iş bittiğinde user-space 
kopyası kullanılmaya devam edilir. Bu durum, kernel-space bellek ile user-space belleği birbirinden
tamamen ayrır. Bu şekilde saldırganın bir zafiyet aracılığı ile kernel'i user-space'e eriştirme 
olanağı tamamen ortadan kalkar.
KAISER kernel parametrelerine `nopti` eklenerek kapatılabilir.

Kernel korumaları oldukça gelişmiş ve korkutucu görünse de bypass etmek için farklı yöntemler mevcut,
ancak merak etmeyin, kafanızı bunlara yormanıza gerek yok, bizim lab ortmamızda bu korumalar 
mevcut olmayacak, en azından stack cookieleri dışında, fakat onu kolayca bypass edeceğiz.

### Kernel panic
Kernel eğer kendini kurtaramıyacağı bir duruma düşerse **B**lack **S**screen **O**f **D**eath (BSOD)
ortaya çıkar. UNIX sistemlerine özgü olan bu ekran linux sistemlerinde bize aslında oldukça bilgi 
verebilir, bu bilgiler arasında stack durumu, temel registerlar ve değerleri ve benzeri birçok 
programın durumunu açıklayan öge bulunur.

### Kernel modules
Kernel modülleri, kernel'e runtime yani çalışma sırasında eklenebilen/kaldırılabilen, derlenmiş
programlardır. Kernel modülleri genelde driver yüklemek adına kullanılır, bu modüller kernel 
fonksiyonlarına erişebilir ve doğal olarak kernel-space'de çalışır.

Neden modüllerden bahsetiğimi merak ediyorsanız, exploit ediceğimiz zafiyet bir modülde bulunuyor da 
ondan. Kernel'in kaynağını doğrudan değiştirip bir zafiyet eklemek, zafiyetli bir kernel modülü
yazmaktan çok daha fazla uğraş gerektiren ve dağıtımı daha zor olan bir durum. Bu yüzden sadece 
bu challenge değil, başka CTF challengelarında da zafiyetli modüller ile çalışıyor olacağınızdan 
bu modüllerin ne olduğunu bilmekte fayda var.

İlerki bölümlerde zafiyetli modülümüzü incelerken daha fazla detaylı olarak modüllerden bahsedeceğiz.

---
[Önceki](README.md) | [Sonraki](setup.md)
