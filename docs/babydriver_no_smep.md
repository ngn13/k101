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




