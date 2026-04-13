; Hello World — Amstrad CPC
; Assemblé avec RASM
; Lancer avec : CALL &8000

        BANKSET 0       ; nécessaire pour que RASM génère le snapshot CPC
        ORG     #8000
        RUN     start

; ─── Point d'entrée ──────────────────────────────────────────────────────────
start:
        ld      hl, msg_hello
        call    print_string

        ; Boucle infinie (point d'arrêt naturel en debug)
loop:
        jr      loop

; ─── Affiche une chaîne terminée par 0 ───────────────────────────────────────
; Entrée : HL → chaîne
print_string:
        ld      a, (hl)
        or      a
        ret     z
        call    TXT_OUTPUT      ; firmware CPC : affiche le caractère dans A
        inc     hl
        jr      print_string

; ─── Constantes firmware ─────────────────────────────────────────────────────
TXT_OUTPUT      EQU     #BB5A

; ─── Données ─────────────────────────────────────────────────────────────────
msg_hello:
        db      "Hello, World!", 13, 0

breakpoint EXEC,READ,STOP,ADDR=print_string