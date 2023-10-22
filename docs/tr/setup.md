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
değilseniz** önceden inşa edilmiş bir root dosya sistemini 
[burdan](https://files.ngn13.fun/k101/root.tar.gz) indirebilirsiniz.

[Bu araçları kurduktan sonra](https://www.kernel.org/doc/html/latest/process/changes.html)
kernel'i indirmek ve derlemek için `kernel.sh` scriptini çalıştırın. Dilerseniz `vars.sh` 
scripti ile oynayarak kernel versiyonunu değiştirebilirsiniz.

Root sistemi hazırladıktan sonra ve kernel'i başarı ile derledikten sonra sistemi başlatmak adına 
`qemu.sh` scriptini çalıştırın. Kernele verilen argümanlar ile oynamak adına yine `vars.sh` 
scripti ile oynuyabilirsiniz

---
[Önceki](kernel.md) | [Sıradaki](deeper.md)
