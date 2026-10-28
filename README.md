# bertil

SW for Arduino Uno R4 WiFi.

Arduino Uno R4 WiFi som slår på en klocka med ett SG90-servo vid bestämda klockslag. Tiden hämtas över WiFi från NTP.

## 1. Installera

1. Installera **VS Code** och tillägget **PlatformIO IDE**.
2. Öppna repots mapp (File → Open Folder). PlatformIO hämtar stödet för Uno R4 WiFi, Servo, RTClib (för datumberäkningar) och hd44780 (skärmen) automatiskt vid första bygget.

### WiFi-uppgifter

Kopiera `include/Secrets.example.h` till `include/Secrets.h` och fyll i nätverkets namn och lösenord. `Secrets.h` checkas inte in. Utan den går det inte att bygga.

Uno R4 WiFi klarar bara 2,4 GHz-nätverk.

## 2. Koppla in

| Från                  | Till        |
|-----------------------|-------------|
| Servo signal (orange) | D9          |
| Servo + (röd)         | 5V          |
| Servo GND (brun)      | GND         |
| LCD1602 (I2C) VCC     | 5V          |
| LCD1602 (I2C) GND     | GND         |
| LCD1602 (I2C) SDA     | SDA (A4)    |
| LCD1602 (I2C) SCL     | SCL (A5)    |

Skärmen är en LCD1602 med I2C-adapter (PCF8574) på baksidan. Adressen hittas automatiskt. Syns ingenting, vrid på kontrastskruven på adaptern. Utan skärm skriver konsolen `Fel: hittar ingen skärm.` och resten fungerar som vanligt.

Sätt en 470 µF-kondensator mellan servots 5V och GND, med minusbenet mot GND.

Montera inte slagarmen ännu. Servot åker till viloläget vid start och vinklarna är inte inställda.

## 3. Ladda upp och öppna konsolen

Klicka på **Upload** (pilen) och sedan **Serial Monitor** (kontakten) i PlatformIO:s statusrad längst ner, eller kör:

```
pio run -t upload
pio device monitor
```

Konsolen skriver `Klockslagare redo. Skriv 'help' för kommandon.` och `Väntar på tid från NTP.` Efter några sekunder kommer `WiFi ansluten till ..., IP ...` och sedan `NTP: <lokal tid>  rundtur ... ms`.

Kommer `WiFi: hittar inte nätverket` eller `anslutningen misslyckades`, kontrollera namn och lösenord i `include/Secrets.h`. Kortet försöker igen var 30:e sekund.

## 4. Klockan

Klockan ställs automatiskt från NTP (`pool.ntp.org`) så fort kortet kommit ut på nätet, och sedan var tionde minut. Kortets egen klocka saknar kristall och kan gå fel några sekunder i timmen, därför synkas den ofta. Sommartid sköts automatiskt.

```
time
wifi
```

`wifi` visar när klockan senast synkades och hur mycket den då korrigerades. `sync` synkar direkt.

Utan nätverk går det att ställa klockan för hand, med svensk tid som den står på väggen. Nästa lyckade NTP-synk skriver över den.

```
set 2026-10-26 14:05:00
```

Efter en omstart finns ingen tid förrän NTP svarat (eller `set` använts), och tills dess slår klockan inte enligt schemat.

## 5. Trimma slaget

1. Montera armen på servot i viloläget.
2. Skriv `strike` och se hur långt armen går.
3. Justera i `include/Config.h`:
   - `kRestAngleDeg`: viloläge, en bit från klockan.
   - `kStrikeAngleDeg`: där servot stannar, **strax före** klockan. Tråden fjädrar sista biten och slår.
4. Ladda upp igen och testa. Ändra några grader i taget.

Rör sig armen åt fel håll, byt värdena eller sätt `kStrikeAngleDeg` lägre än `kRestAngleDeg`.

## 6. Ställ in schemat

Ändra i `include/Config.h` och ladda upp igen:

```cpp
constexpr StrikeTime kSchedule[] = {
    {9, 30, 1},    // 09:30, ett slag
    {12, 0, 3},    // 12:00, tre slag
    {15, 45, 1},
};
```

`schedule` i konsolen visar vad som ligger inne.

Tidszonen ställs med `kUtcOffsetHours` och `kUseEuDst` i samma fil.

## 7. Drift

Koppla in en 5V USB-laddare (minst 1 A). Kortet kör av sig självt utan dator.

Skärmen visar `Nästa slag hh:mm` och `om hh:mm:ss` till nästa klockslag, och `Slår nu!` medan det slår. Innan tiden är hämtad står `Väntar på tid från NTP`.

Lysdioden på kortet:

- **Lugn blink:** allt fungerar.
- **Dubbelblink:** klockan går, men har inte kunnat synkas på över en timme. Kontrollera WiFi med `wifi`.
- **Snabb blink:** klockan har ingen tid än. Vänta på NTP, eller använd `set`.

## Seriekommandon (115200 baud)

| Kommando                  | Funktion                |
|---------------------------|-------------------------|
| `time`                    | visa tiden              |
| `set ÅÅÅÅ-MM-DD hh:mm:ss` | ställ klockan           |
| `strike` / `strike 3`     | slå direkt              |
| `schedule`                | visa schemat            |
| `wifi`                    | visa WiFi och NTP       |
| `sync`                    | synka klockan nu        |
| `help`                    | lista kommandon         |

Om kortet startar om när konsolen öppnas åker servot till viloläget och klockan måste ställas igen.

## Skisser

- `docs/slagarm.svg`: slagarmen med pianotråd och vikt
- `docs/infastning.svg`: infästningen mellan servoarm och tråd
