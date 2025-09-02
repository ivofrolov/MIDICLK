;; POWER
;; see https://doepfer.de/a100_man/a100t_e.htm
(+5V)
(+12V)
(GND)

;; MIDI INPUT
;; Type A (MIDI standard) connector, see https://minimidi.world/
;; (J2 AudioJack :pins    S         R    T   )
;; (J1 DIN       :pins NC SHIELD NC VREF DATA)
;;                     1  2      3  4    5
;; see https://learn.sparkfun.com/tutorials/midi-tutorial/hardware--electronic-implementation
;; see https://cdn.sparkfun.com/datasheets/Dev/Arduino/Shields/Midi_Shieldv15.pdf
(J2 AudioJack :pins T R NC)
((J2 :pins R) (R1 220) (U2 :pins +))
((J2 :pins T) (U2 :pins -))
((U2 :pins +) (D1 1N914 :pins K A) (U2 :pins -))

(U2 6N138 :pins NC + - NC GND VO NC +5V)
((U2 :pins VO) (R2 200) +5V)
((U2 :pins VO) (U1 :pins PB0))

;; INPUT ENCODER
;; see https://cdn.sparkfun.com/assets/1/8/1/7/6/Qwiic_Twist_Schematic.pdf
(ENCOUT
 ((ENC (= ((R1 10K) +5V)
          ((R2 10K) (= ((C1 10n) GND)
                       OUT)))))
 :pins ENC OUT)
(SW1 RotaryEncoder :pins A C B S1 S2)
((SW1 :pins A) ENCOUT (U1 :pins PB3))
((SW1 :pins C) GND)
((SW1 :pins B) ENCOUT (U1 :pins PB4))

;; MCU
(U1 ATtiny85 :pins PB5 PB3 PB4 GND PB0 PB1 PB2 +5V)
((U1 :pins +5V) (C1 10u) (U1 :pins GND))
;; (+5V (U1 :pins +5V))
;; (GND (U1 :pins GND))

;; CLOCK OUTPUT
(CLKOUT
 ((Q1 2N3904 :pins E B C)
  (IN (R1 1K) (Q1 :pins B))
  (+12V (R2 1K) (Q1 :pins C))
  (GND (Q1 :pins GND))
  ((Q1 :pins C) (R3 100) (= ((R4 1K) GND)
                            ((D1 LED :pins A K) (R5 1K) GND)
                            OUT)))
 :pins IN OUT)

((U1 :pins PB1) (OUTA CLKOUT :pins IN OUT) (J3 AudioJack :pins T))
((J3 :pins S) GND)
((U1 :pins PB2) (OUTB CLKOUT :pins IN OUT) (J4 AudioJack :pins T))
((J4 :pins S) GND)
