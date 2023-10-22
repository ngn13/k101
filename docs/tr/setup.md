# Lab ortamını ayarlamak
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
**Eğer Arch tabanlı bir dağıtımdaysanız** önce `arch-install-scripts`'i kurup ardından `root.sh` 
scriptini çalıştırarak root dosya sistemini inşa edebilirsiniz. **Eğer arch tabanlı bir dağıtımda 
değilseniz** önceden inşa edilmiş bir root dosya sistemini isterseniz:
- [root.tar.gz](https://files.ngn13.fun/k101/root.tar.gz) (1.7G - Arşivden çıkarınca 6GB)
- **SHA256 imzası**: `5df4f614b812a32483ea55ad0bd1b773ac543af74b1d8ac11463f14f0e952b38`

[Bu araçları kurduktan sonra](https://www.kernel.org/doc/html/latest/process/changes.html)
kernel'i indirmek ve derlemek için `kernel.sh` scriptini çalıştırın. Dilerseniz `vars.sh` 
scripti ile oynayarak kernel versiyonunu değiştirebilirsiniz.

Root sistemi hazırladıktan sonra ve kernel'i başarı ile derledikten sonra sistemi başlatmak adına 
`qemu.sh` scriptini çalıştırın. Kernele verilen argümanlar ile oynamak adına yine `vars.sh` 
scripti ile oynuyabilirsiniz

Sistem açılınca **kullanıcı adı olarak** `root` ve **parola olarak** `k101root` ile giriş yapabilirsiniz,
ancak daha stabil bir shell istiyorsanız aynı kullanıcı ve parola ile port 2222'de SSH'a bağlanın:
```
ssh root@127.0.0.1 -p 2222
```
Benim önerim SSH'ı kullanmanız olacaktır. 

Sisteme girdikten sonra zafiyetli sırdaki bölümde inceleyeceğimiz kernel modülünü ve en son yazıcağımız 
root exploitini root'un ev dizininde (`/root`) bulabilirsiniz. Exploitimizi aşama aşama yazacağımızdan size 
önerim kendi exploitiniz için ayrı bir dizin oluşturmanızdır.

Ve evet makinede `vim` editörü ile beraber exploitinizi derlemek için gerekli olacak temel araçlar mevcut.
Yani onları indirmek ile uğraşmak zorunda değilsiniz.

---
[Önceki](kernel.md) | [Sıradaki](deeper.md)
