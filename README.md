# bertil

SW for Arduino Uno rev3, using after market RTC.

Arduino Uno R3 som slår på en klocka med ett SG90-servo vid bestämda klockslag. Tiden hålls av en Adafruit DS3231.

## Koppling

| Från            | Till               |
|-----------------|--------------------|
| Servo signal    | D9                 |
| Servo +         | 5 V                |
| Servo GND       | GND                |
| DS3231 VIN      | 5 V                |
| DS3231 GND      | GND                |
| DS3231 SDA      | A4                 |
| DS3231 SCL      | A5                 |
| 470 µF          | över servots 5 V och GND |

Sätt i ett CR1220-batteri i RTC-modulen så att tiden finns kvar utan ström.

## Bygga och ladda upp

```
pio run -t upload
pio device monitor
```

## Ställa in

Allt som ska justeras finns i `include/Config.h`:

- `kRestAngleDeg` och `kStrikeAngleDeg`: vilo- och slagvinkel
- `kSchedule`: klockslag och antal slag
- `kUtcOffsetHours` och `kUseEuDst`: tidszon och sommartid

## Seriekommandon (115200 baud)

| Kommando                    | Funktion                    |
|-----------------------------|-----------------------------|
| `time`                      | visa lokal tid              |
| `set 2026-10-23 09:30:00`   | ställ klockan (lokal tid)   |
| `strike` / `strike 3`       | slå direkt, för att trimma vinklar |
| `schedule`                  | visa schemat                |
| `temp`                      | visa RTC-temperatur         |

## Lysdiod

- Lugn blink: allt fungerar
- Snabb blink: RTC svarar inte

## Skisser

- `docs/slagarm.svg`: slagarmen med pianotråd och vikt
- `docs/infastning.svg`: infästningen mellan servoarm och tråd
