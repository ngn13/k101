# Lab ortamını kurmak 
Makinenizde hali hazırda bir KVM/QEMU sanallaştırma ortamınız olduğunu varsayıyor 
olacağım, ancak yoksa, distronuzdaki `qemu` ve `libvirt` paketlerini kurarak bunu yapabilirsiniz.

Ardından bu repoyu (hala yapmadıysanız) klonlayın:
```bash
git clone https://github.com/ngn13/k101.git
```
İndirdikten sonra `src` dizinine geçiş yapın:
```bash
cd k101/src
```
Şimdi sıra root dosya sistemi ve de kerneli inşa etmekte:
- **Kerneli derlemek:** [Bu araçları kurduktan sonra](https://www.kernel.org/doc/html/latest/process/changes.html), 
`kernel.sh` scriptini çalıştırmanız yeterli, dilerseniz `vars.sh` scripti ile oynayarak versiyonu değiştirebilirsiniz
- **Root dosya sistemini inşa etmek:** `root.sh` scriptini çalıştırmanız yeterli, bu script
`arch-install-scripts` üzerinden çalıştığından **sadece arch tabanlı dağıtımlarda çalışacaktır.**

**Arch tabanlı bir dağıtımda değilseniz** ya da basitçe **önceden derlenmiş dosya sistemi ve de kerneli
tercih ediyorsanız**, o zaman yukarıda bahsi geçen ile inşa edilmiş hazır dosya sistemini ve de
kernel'i kullanabilirsiniz:

- [k101.tar.gz](https://files.ngn.tf/p/k101.tar.gz) (1.7G - Arşivden çıkarınca 6GB)
- **PGP imzası**: [k101.tar.gz.sig](https://files.ngn.tf/p/k101.tar.gz.sig) 
- **SHA256 imzası**: `f287c43c975a072df01a719ee614cb4f2c2b0e80d1b8a00da5ec8d77a04b0631`

Root sistemi hazırladıktan sonra ve kernel'i başarı ile derledikten sonra sistemi başlatmak adına 
`qemu.sh` scriptini çalıştırın. Kernele verilen argümanlar ile oynamak adına yine `vars.sh` 
scripti ile oynuyabilirsiniz

Sistem açılınca **kullanıcı adı olarak** `root` ve **parola olarak** `k101root` ile giriş yapabilirsiniz,
ancak daha stabil bir shell istiyorsanız aynı kullanıcı ve parola ile port 2222'de SSH'a bağlanın:
```bash
ssh root@127.0.0.1 -p 2222
```
Benim önerim SSH'ı kullanmanız olacaktır. 

Sisteme girdikten sonra zafiyetli sırdaki bölümde inceleyeceğimiz kernel modülünü
root'un ev dizininde (`/root`) bulabilirsiniz. Exploitimizi aşama aşama yazacağımızdan size 
önerim kendi exploitiniz için ayrı bir dizin oluşturmanızdır.

Ve evet makinede `vim` editörü ve `tmux` çoklu pencere yöneticisi ile beraber exploitinizi 
derlemek için gerekli olacak temel araçlar mevcut. Yani onları indirmek ile uğraşmak zorunda değilsiniz.

---
[Önceki](kernel.md) | [Sıradaki](deeper.md)
