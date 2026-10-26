# bertil

Arduino Uno R3 som slår på en klocka med ett SG90-servo vid bestämda klockslag. Tiden hålls av en Adafruit DS3231.

## 1. Installera

1. Installera **VS Code** och tillägget **PlatformIO IDE**.
2. Öppna repots mapp (File → Open Folder). PlatformIO hämtar Servo och RTClib automatiskt vid första bygget.

## 2. Koppla in

| Från                  | Till        |
|-----------------------|-------------|
| Servo signal (orange) | D9          |
| Servo + (röd)         | 5V          |
| Servo GND (brun)      | GND         |
| DS3231 VIN / GND      | 5V / GND    |
| DS3231 SDA / SCL      | A4 / A5     |

Sätt en 470 µF-kondensator mellan servots 5V och GND, med minusbenet mot GND. Sätt i ett CR1220-batteri i RTC-modulen så att tiden finns kvar utan ström.

Montera inte slagarmen ännu. Servot åker till viloläget vid start och vinklarna är inte inställda.

## 3. Ladda upp och öppna konsolen

Klicka på **Upload** (pilen) och sedan **Serial Monitor** (kontakten) i PlatformIO:s statusrad längst ner, eller kör:

```
pio run -t upload
pio device monitor
```

Konsolen skriver `Klockslagare redo. Skriv 'help' för kommandon.`

## 4. Ställ klockan

```
set 2026-10-26 14:05:00
time
```

Ange svensk tid som den står på väggen. Sommartid sköts automatiskt och batteriet håller tiden när strömmen är borta. Första gången sätts tiden till när koden kompilerades, vilket är några sekunder fel, så använd `set` för att ställa exakt.

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

Lysdioden på kortet:

- **Lugn blink:** allt fungerar.
- **Snabb blink:** RTC:n svarar inte. Kontrollera SDA, SCL och matning.

## Seriekommandon (115200 baud)

| Kommando                  | Funktion                |
|---------------------------|-------------------------|
| `time`                    | visa tiden              |
| `set ÅÅÅÅ-MM-DD hh:mm:ss` | ställ klockan           |
| `strike` / `strike 3`     | slå direkt              |
| `schedule`                | visa schemat            |
| `temp`                    | visa RTC-temperatur     |
| `help`                    | lista kommandon         |

Kortet startar om när konsolen öppnas och servot åker då till viloläget. Det är normalt.

## Skisser

- `docs/slagarm.svg`: slagarmen med pianotråd och vikt
- `docs/infastning.svg`: infästningen mellan servoarm och tråd
