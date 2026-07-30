# Pardus Dikte

Sistem genelinde, çevrimdışı sesli yazma aracı — *system-wide, offline voice dictation.*

Bir kısayola basın, konuşun; söyledikleriniz siz konuşurken bir not defteri penceresine yazılır. Tamamen internetsiz ve yerel çalışır.

---

## Türkçe

### Nedir?

Pardus Dikte, `Super+Shift+D` kısayoluna bastığınızda karşınıza bir not defteri penceresi açan, mikrofonu dinlemeye başlayan ve siz konuşurken sesinizi yerel bir Whisper modeliyle canlı olarak metne çeviren küçük bir masaüstü aracıdır. Metin siz konuşurken pencerede belirir; dilediğiniz gibi düzenleyip kopyalayabilirsiniz.

Windows 11'deki `Win+H` dikte kutusunu düşünün: kurulum yok, ayar yok, bas-konuş-yazılsın. Referans noktamız tam olarak budur.

### Neden gerekli?

Windows'ta `Win+H` ile gelen sıfır-kurulumlu dikte özelliğinin Linux masaüstünde bir karşılığı yok. Ne GNOME ne de Pardus, kutudan çıktığı haliyle böyle bir şey sunuyor. Buna en çok yaklaşan araçlar (örneğin Vosk tabanlı `nerd-dictation`) terminal kullanmayı, elle yapılandırma dosyası düzenlemeyi ve konuşma modelini kendiniz bulup indirmeyi gerektiriyor — teknik olmayan bir kullanıcı için erişilemez. Linux'ta bugün "tek tıkla, her yerde, anında" çalışan keşfedilebilir bir çözüm yok. Pardus Dikte bu boşluğu doldurmak için var.

Aracın asıl değeri transkripsiyon teknolojisi değildir; yerel Whisper zaten yaygın bir teknoloji. Asıl değer **hiç ayar gerektirmeden, sürtünmesiz çalışmasıdır.**

### Nasıl çalışır?

1. Kurulumdan sonra `Super+Shift+D` kısayolu GNOME'a otomatik olarak tanıtılır (siz elle bağlamazsınız).
2. Kısayola bastığınızda not defteri penceresi açılır ve dinleme hemen başlar.
3. Mikrofon dinlenir, gürültü bastırma uygulanır; ses arka planda sürekli olarak yerel Whisper modeliyle Türkçe metne çevrilir.
4. Metin, siz konuşurken pencerede canlı olarak belirir. Yazım sürerken metni düzenleyebilirsiniz.
5. Dinleme, siz "Dinlemeyi durdur" düğmesine basana kadar sürer — düşünmek için verdiğiniz molalar kaydı kesmez. Durdurduğunuzda son bir düzeltme geçişi yapılır; metni "Kopyala" ile panoya alabilirsiniz.

Arka planda sürekli çalışan bir servis yoktur. Uygulama yalnızca kısayola bastığınızda çalışır, işini bitirince tamamen kapanır.

### Kurulum

`.deb` paketini kurmak yeterlidir; başka hiçbir terminal adımı, ayar veya model indirme gerekmez:

```
sudo dpkg -i pardus-dikte_1.0.0_amd64.deb
sudo apt -f install    # eksik bağımlılık olursa
```

Kurulumdan hemen sonra `Super+Shift+D` çalışmaya hazırdır.

### Kaynaktan derleme

Gerekli paketler:

```
sudo apt install cmake ninja-build g++ qt6-base-dev qt6-declarative-dev libwebrtc-audio-processing-dev pkg-config
```

Derleme (Whisper ve model derleme sırasında otomatik indirilir; internet gerekir):

```
cmake -B build -G Ninja
cmake --build build -j$(nproc)
cpack --config build/CPackConfig.cmake    # .deb üretmek için
```

### v1 kapsamı (dürüst sınırlar)

Bu sürüm bilinçli olarak dar tutulmuştur:

- **Yalnızca klavyeli masaüstü/dizüstü Pardus makineleri.** Fiziksel klavyesi olmayan ETAP akıllı tahtalar bu sürümde desteklenmez; o senaryo sesle tetikleme gerektirir ve gelecekteki bir sürümün konusudur.
- **Tetikleyici klavye kısayoludur, "uyandırma kelimesi" değildir.** Özel bir Türkçe uyandırma kelimesi eğitmek başlı başına bir makine öğrenmesi projesidir ve v2 yol haritasındadır.
- **Sadece transkripsiyon.** Yapay zekâ ile komut anlama, eylem çalıştırma veya internet bağlantısı yoktur. Ses girer, metin çıkar.

### Lisans

GPL-3.0. Ayrıntılar için `LICENSE` dosyasına bakın.

---

## English

### What is it?

Pardus Dikte is a small desktop utility. Press `Super+Shift+D` and a notepad window opens, listens to your microphone, and transcribes your speech live with a local Whisper model as you talk. The text appears in the window while you speak; you can edit or copy it freely. Think of Windows 11's `Win+H` dictation box: no setup, no configuration, just press-speak-type.

### Why it is needed

No mainstream Linux desktop ships a zero-setup, system-wide dictation feature comparable to `Win+H`. Neither GNOME nor Pardus offers this out of the box. The closest tools (such as the Vosk-based `nerd-dictation`) require terminal usage, hand-editing config files, and manually sourcing a speech model — inaccessible to non-technical users. There is no discoverable, "just works" answer on Linux today. Pardus Dikte fills that gap.

The transcription technology (local Whisper) is not the hard part — it is commoditized. The entire value here is that it requires **zero configuration and zero friction.**

### How it works

1. After installation the `Super+Shift+D` shortcut is registered with GNOME automatically — you never bind it by hand.
2. Pressing it opens a notepad window and listening starts immediately.
3. The microphone is captured, noise suppression is applied, and the audio is continuously transcribed locally with a Whisper model (Turkish) in the background.
4. Text appears live in the window as you speak. You can edit it while transcription continues.
5. Listening continues until you press "Dinlemeyi durdur" (Stop listening) — pausing to think never cuts the recording short. A final cleanup pass runs when you stop; use "Kopyala" (Copy) to put the text on your clipboard.

There is no persistent background daemon. The binary is not resident in memory between uses; GNOME launches it fresh each time the shortcut fires, and it exits when the window is closed.

If the notepad window fails to open for any reason, the app falls back to transcribing headlessly, copying the result to the clipboard, and showing a short notice explaining that the window couldn't be opened.

### Install

```
sudo dpkg -i pardus-dikte_1.0.0_amd64.deb
sudo apt -f install    # if any dependency is missing
```

Nothing else — no terminal commands to run, no config files to edit, no model to download separately. `Super+Shift+D` works immediately.

### Build from source

```
sudo apt install cmake ninja-build g++ qt6-base-dev qt6-declarative-dev libwebrtc-audio-processing-dev pkg-config
cmake -B build -G Ninja
cmake --build build -j$(nproc)
cpack --config build/CPackConfig.cmake
```

Whisper.cpp and the model are fetched automatically during the build (internet required at build time only).

### v1 scope (honest boundaries)

This release is intentionally narrow:

- **Desktop/laptop Pardus machines with a keyboard only.** ETAP smart boards without a physical keyboard are not supported in v1 — that scenario needs a voice trigger and is a credible future direction, not part of this build.
- **The trigger is a keyboard shortcut, not a wake word.** Training a custom Turkish wake word is a machine-learning project of its own and is on the v2 roadmap.
- **Transcription only.** No LLM, no understanding of what was said, no actions, no network. Audio in, text out.

### License

GPL-3.0. See the `LICENSE` file.
