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
| LCD 1 VSS             | GND         |
| LCD 2 VDD             | 5V          |
| LCD 3 V0 (kontrast)   | mittbenet på en 10 kΩ potentiometer, ytterbenen till 5V och GND |
| LCD 4 RS              | D12         |
| LCD 5 RW              | GND         |
| LCD 6 E               | D11         |
| LCD 7-10 D0-D3        | kopplas inte |
| LCD 11 D4             | D5          |
| LCD 12 D5             | D4          |
| LCD 13 D6             | D3          |
| LCD 14 D7             | D2          |
| LCD 15 A (bakljus +)  | 5V via 220 Ω |
| LCD 16 K (bakljus -)  | GND         |

Skärmen är en LCD1602 utan I2C-adapter, kopplad i 4-bitarsläge så att bara 4 av de 8 datakablarna behövs. Syns ingenting, vrid på potentiometern. Står det bara svarta rutor på översta raden får skärmen ström men ingen data, kontrollera RS, E och D4-D7. Pinnarna ändras med `kLcd...Pin` i `include/Config.h`.

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

Vilka veckodagar klockan slår styrs av `kStrikeDays` i samma fil. Som standard slår den måndag till fredag och är tyst på helgen:

```cpp
constexpr bool kStrikeDays[7] = {
    false,  // sön
    true,   // mån
    ...
    false,  // lör
};
```

`schedule` i konsolen visar vad som ligger inne.

Tidszonen ställs med `kUtcOffsetHours` och `kUseEuDst` i samma fil.

### Citat på skärmen

Citaten står i `include/quotes.h`. Varje citat har två delar:

```cpp
{"You will be okay", "You have no choice"},
```

Skärmen går i en cykel som börjar om varje hel minut:

| Sekund | Visas                       |
|--------|-----------------------------|
| 00-19  | tid till nästa slag         |
| 20-29  | citatets första del         |
| 30-39  | citatets andra del          |
| 40-49  | första delen igen           |
| 50-59  | andra delen igen            |

Nästa minut visas nästa citat i listan. Texten bryts automatiskt över två rader à 16 tecken. Blir en del längre än två rader bläddras den en rad i taget. Ord som är längre än 16 tecken delas med bindestreck. `å ä ö Å Ä Ö` fungerar, andra specialtecken visas som `?`. Ett `"` i texten skrivs som `\"`.

Tiderna ställs med `kCountdownSeconds` och `kQuotePartSeconds` i `include/Config.h`.

## 7. Drift

Koppla in en 5V USB-laddare (minst 1 A). Kortet kör av sig självt utan dator.

Skärmen visar `Nästa slag hh:mm` och `om hh:mm:ss` till nästa klockslag i början av varje minut, och sedan citat (se avsnitt 6). Medan det slår står `Slår nu!`. Innan tiden är hämtad står `Väntar på tid från NTP` i stället för nedräkningen.

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
