; PUTSPR_.ASM - rysuje sprajty na ekranie

IDEAL
INCLUDE         "common.asi"

DATASEG

GLOBAL C rle : rleinfo
GLOBAL C cc_result : PTR BYTE

cc_temp   DB 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
          DB 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
          DB 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
          DB 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
          DB 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
          DB 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
          DB 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
          DB 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0

STRUC rleinfo
  x dd ?
  y dd ?
  sx dd ?
  sy dd ?
  mirror dd ?
  eos dd ?
  def dd ?
  scr_sx dd ?
  scr_sy dd ?
  scr_llen dd ?
  scr_adr dd ?
ENDS

CODESEG
LOCALS
JUMPS

MACRO           getcount reg
  movsx reg,[word ptr esi]
  lea esi,[esi+2]
ENDM

PROCEDURE       _uniput
ARG             doit :DWORD, using :DWORD
LOCAL           toskip: DWORD, todraw: DWORD, llen :DWORD
LOCAL           esi_sav: DWORD, edi_sav: DWORD, ecx_sav: DWORD, ebx_sav: DWORD

  push eax
  push ebx
  push ecx
  push edx
  push ebp
  push esi
  push edi

  cld

  mov eax,[rle.scr_llen]           ; EAX = długość linii ekranu
  mov [llen],eax
  imul eax,[rle.y]                      ; EAX *= pozycja y sprajta
  add eax,[rle.scr_adr]            ; EAX = adres początku linii
  add eax,[rle.x]
  mov edi,eax                           ; EAX = pointer do ekranu

  test [rle.mirror],02h
  $if nz
    mov eax,[rle.sy]
    dec eax
    imul eax,[llen]
    add edi,eax
  $endif

  mov esi,[rle.def]
  mov ecx,[rle.eos]
  cmp ecx,4
  $if ne
    push ecx

    test [rle.mirror],02h
    $if z                               ; bez lustra pionowego
      cmp [rle.y],0                         ; pozycja y mniejsza od zera?
      $if l                             ; tak: obcinać na górze
        mov ecx,[rle.y]
        neg ecx                         ; ECX = ile góry obciąć
        sub [rle.sy],ecx                    ; zmniejsz wysokość sprajta
        add [rle.y],ecx                     ; zwiększ pozycję y
        mov ebx,[llen]                  ; EDX = długość linii ekranowej
        $do                             ; opuść ECX linii
          getcount eax                  ; EAX = długość linii definicji
          add esi,eax                   ; opuść linię sprajta
          add edi,ebx                   ; opuść linię ekranową
        $enddo loop                     ; następna linia
      $endif

      mov eax,[rle.y]                       ; EAX = pozycja y sprajta
      add eax,[rle.sy]                      ; EAX = pozycja dolnej krawędzi
      sub eax,[rle.scr_sy] ; cmp
      $if g                             ; obcinać na dole?
        sub [rle.sy],eax                    ; zmniejsz wysokość - to wystarczy
      $endif

    $else                               ; z lustrem pionowym
      mov eax,[rle.y]                       ; EAX = pozycja y sprajta
      add eax,[rle.sy]                      ; EAX = pozycja dolnej krawędzi
      sub eax,[rle.scr_sy] ; cmp
      $if g                             ; obcinać na dole?
        mov ecx,eax
        sub [rle.sy],ecx                    ; zmniejsz wysokość sprajta
        mov ebx,[llen]                  ; EDX = długość linii ekranowej
        $do                             ; opuść ECX linii
          getcount eax                  ; EAX = długość linii definicji
          add esi,eax                   ; opuść linię sprajta
          sub edi,ebx                   ; opuść linię ekranową
        $enddo loop                     ; następna linia
      $endif

      cmp [rle.y],0                         ; pozycja y mniejsza od zera?
      $if l                             ; tak: obcinać na górze
        mov eax,[rle.y]                     ; ujemne
        add [rle.sy],eax                    ; zmniejsz wysokość - to wystarczy
      $endif
    $endif

    cmp [rle.x],0                         ; pozycja x mniejsza od zera?
    $if l                             ; tak: obcinać z lewej
      mov ecx,[rle.x]                     ; ECX = -(ile nie rysować)
      mov [rle.x],0                       ; przesuń pozycję x
    $else                             ; nie: nic nie obcinać
      xor ecx,ecx                     ; ECX = 0
    $endif
    mov [toskip],ecx                  ; [toskip] = -(ile nie rysować)

    mov eax,[rle.scr_sx]         ; EAX = szerokość ekranu
    sub eax,[rle.x]                       ; EAX = ile sprajta się zmieści
    mov [todraw],eax                  ; ile wyrysować (obcinanie z prawej)

    pop ecx
  $endif

; **** rysuj sprajta normalnie

  cmp [doit],0
  $if e
    cmp ecx,4                           ; ECX = ile krawędzi jest na ekranie
    $if e                               ; cały sprajt (4 krawędzie)
      $do                               ; rysuj [rle.sy] linii
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount ebx                  ; EBX = ilość bloków
          $do                           ; rysuj [bx] bloków
            getcount eax                ; AEX = ile opuścić
            add edi,eax                 ; opuść EAX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            cmp ecx,0                   ; sprawdź rodzaj bloku
            $if g                       ; blok do skopiowania
              fast_movsb                ; kopiuj bajty
            $else                       ; blok do powielenia
              lodsb                     ; kolor do powielenia
              neg ecx                   ; ECX = ile razy powielić
              fast_stosb                ; powiel bajt
            $endif
            dec ebx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        cmp [word ptr esi],0ffffh
      $enddo jne                        ; następna linia
    $else                               ; sprajt z obcinaniem
      $do
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount ebx                  ; EBX = ilość bloków

          mov edx,[toskip]              ; EDX = -(ile nie rysować)
          $do                           ; opuść EDX pikseli
            getcount ecx                ; ECX = ile opuścić
            add edx,ecx
            $if ns                      ; już na ekranie?
              sub edx,ecx               ; cofnij się poza ekran
              neg edx                   ; EDX = ile pikseli do granicy ekranu
              add edi,edx               ; EDI = lewy brzeg ekranu
              sub ecx,edx               ; ile opuścić
              getcount eax              ; ile kopiować
              xchg ecx,eax              ; EAX = opuścić, ECX = kopiować
              $leave                    ; lewy brzeg obcięty, rysuj sprajta
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            test ecx,ecx                ; sprawdź rodzaj bloku
            $if ns                      ; jeśli (+) kopiowanie bajtów
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                add esi,edx             ; ESI = skąd kopiować
                sub ecx,edx             ; ECX = ile kopiować
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              add esi,ecx               ; opuść ECX pikseli w definicji
            $else                       ; jeśli (-) powielanie bajtów
              neg ecx                   ; ECX = ile razy powielić
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                sub ecx,edx             ; ECX = ile razy powielić
                $if z                   ; jeśli nic do powielenia
                  inc esi               ; opuść powielany kolor
                $endif
                neg ecx                 ; ECX = -(ile razy powielić)
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              inc esi                   ; opuść powielany kolor
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            dec ebx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok

          or ebx,ebx                    ; sprawdź ilość bloków
          $if nz                        ; jeśli są bloki do wyrysowania
            mov edx,[todraw]            ; EDX = ile rysować
            $do
              sub edx,eax ;cmp          ; wszystko narysowane?
              $leave le                 ; jeśli tak, następna linia linia
              add edi,eax               ; opuść EAX pikseli
              cmp ecx,0                 ; sprawdź rodzaj bloku
              $if g                     ; ECX > 0: kopiowanie bajtów
                sub edx,ecx ;cmp        ; wszystko narysowane?
                $if g                   ; nie
                  fast_movsb            ; kopiuj bajty
                $else                   ; tak
                  add edx,ecx           ; EDX = poprzednia wartość
                  mov ecx,edx           ; ECX = ile jeszcze do wyrysowania
                  fast_movsb            ; kopiuj tylko tyle bajtów
                  $leave                ; następna linia
                $endif
              $else                     ; ECX <= 0: powielanie bajtu
                $if ne                  ; ECX == 0: odpuść sobie
                  lodsb                 ; kolor do powielenia
                  neg ecx               ; ECX = ile razy powielić
                  sub edx,ecx ;cmp      ; wszystko narysowane?
                  $if g                 ; nie
                    fast_stosb          ; powiel bajt
                  $else                 ; tak
                    add edx,ecx         ; EDX = poprzednia wartość
                    mov ecx,edx         ; ECX = ile jeszcze do wyrysowania
                    fast_stosb          ; powiel tylko tyle razy
                    $leave              ; następna linia
                  $endif
                $endif
              $endif
              getcount ecx              ; ile opuścić
              getcount eax              ; ile kopiować
              xchg eax,ecx              ; EAX = opuścić, ECX = kopiować
              dec ebx                   ; zmniejsz ilość bloków
            $enddo jnz                  ; następny blok
          $endif
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        dec [rle.sy]
      $enddo jnz                        ; następna linia
    $endif
  $endif

; **** rysuj sprajta w jednym kolorze

  cmp [doit],1
  $if e
    cmp ecx,4                           ; ECX = ile krawędzi jest na ekranie
    $if e                               ; cały sprajt (4 krawędzie)
      $do                               ; rysuj [rle.sy] linii
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount ebx                  ; EBX = ilość bloków
          $do                           ; rysuj [bx] bloków
            getcount eax                ; AEX = ile opuścić
            add edi,eax                 ; opuść EAX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            cmp ecx,0                   ; sprawdź rodzaj bloku
            $if g                       ; blok do skopiowania
              add esi,ecx
              mov eax,[using]
              mov eax,[eax]
              fast_stosb                ; kopiuj bajty
            $else                       ; blok do powielenia
              lodsb                     ; kolor do powielenia
              neg ecx                   ; ECX = ile razy powielić
              mov eax,[using]
              mov eax,[eax]
              fast_stosb                ; powiel bajt
            $endif
            dec ebx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        cmp [word ptr esi],0ffffh
      $enddo jne                        ; następna linia
    $else                               ; sprajt z obcinaniem
      $do
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount ebx                  ; EBX = ilość bloków

          mov edx,[toskip]              ; EDX = -(ile nie rysować)
          $do                           ; opuść EDX pikseli
            getcount ecx                ; ECX = ile opuścić
            add edx,ecx
            $if ns                      ; już na ekranie?
              sub edx,ecx               ; cofnij się poza ekran
              neg edx                   ; EDX = ile pikseli do granicy ekranu
              add edi,edx               ; EDI = lewy brzeg ekranu
              sub ecx,edx               ; ile opuścić
              getcount eax              ; ile kopiować
              xchg ecx,eax              ; EAX = opuścić, ECX = kopiować
              $leave                    ; lewy brzeg obcięty, rysuj sprajta
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            test ecx,ecx                ; sprawdź rodzaj bloku
            $if ns                      ; jeśli (+) kopiowanie bajtów
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                add esi,edx             ; ESI = skąd kopiować
                sub ecx,edx             ; ECX = ile kopiować
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              add esi,ecx               ; opuść ECX pikseli w definicji
            $else                       ; jeśli (-) powielanie bajtów
              neg ecx                   ; ECX = ile razy powielić
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                sub ecx,edx             ; ECX = ile razy powielić
                $if z                   ; jeśli nic do powielenia
                  inc esi               ; opuść powielany kolor
                $endif
                neg ecx                 ; ECX = -(ile razy powielić)
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              inc esi                   ; opuść powielany kolor
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            dec ebx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok

          or ebx,ebx                    ; sprawdź ilość bloków
          $if nz                        ; jeśli są bloki do wyrysowania
            mov edx,[todraw]            ; EDX = ile rysować
            $do
              sub edx,eax ;cmp          ; wszystko narysowane?
              $leave le                 ; jeśli tak, następna linia linia
              add edi,eax               ; opuść EAX pikseli
              cmp ecx,0                 ; sprawdź rodzaj bloku
              $if g                     ; ECX > 0: kopiowanie bajtów
                sub edx,ecx ;cmp        ; wszystko narysowane?
                $if g                   ; nie
                  add esi,ecx
                  mov eax,[using]
                  mov eax,[eax]
                  fast_stosb            ; kopiuj bajty
                $else                   ; tak
                  add edx,ecx           ; EDX = poprzednia wartość
                  mov ecx,edx           ; ECX = ile jeszcze do wyrysowania
                  add esi,ecx
                  mov eax,[using]
                  mov eax,[eax]
                  fast_stosb            ; kopiuj bajty
                  $leave                ; następna linia
                $endif
              $else                     ; ECX <= 0: powielanie bajtu
                $if ne                  ; ECX == 0: odpuść sobie
                  lodsb                 ; kolor do powielenia
                  neg ecx               ; ECX = ile razy powielić
                  sub edx,ecx ;cmp      ; wszystko narysowane?
                  $if g                 ; nie
                    mov eax,[using]
                    mov eax,[eax]
                    fast_stosb          ; kopiuj bajty
                  $else                 ; tak
                    add edx,ecx         ; EDX = poprzednia wartość
                    mov ecx,edx         ; ECX = ile jeszcze do wyrysowania
                    mov eax,[using]
                    mov eax,[eax]
                    fast_stosb          ; kopiuj bajty
                    $leave              ; następna linia
                  $endif
                $endif
              $endif
              getcount ecx              ; ile opuścić
              getcount eax              ; ile kopiować
              xchg eax,ecx              ; EAX = opuścić, ECX = kopiować
              dec ebx                   ; zmniejsz ilość bloków
            $enddo jnz                  ; następny blok
          $endif
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        dec [rle.sy]
      $enddo jnz                        ; następna linia
    $endif
  $endif

; **** rysuj sprajta przez narzędzie

  cmp [doit],2
  $if e
    cmp ecx,4                           ; ECX = ile krawędzi jest na ekranie
    $if e                               ; cały sprajt (4 krawędzie)
      mov ebx,[using]
      $do                               ; rysuj [rle.sy] linii
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount edx                  ; EDX = ilość bloków
          $do                           ; rysuj [bx] bloków
            getcount eax                ; AEX = ile opuścić
            add edi,eax                 ; opuść EAX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            cmp ecx,0                   ; sprawdź rodzaj bloku
            $if g                       ; blok do skopiowania
              fast_xlatesiedi
            $else                       ; blok do powielenia
              lodsb                     ; kolor do powielenia
              neg ecx                   ; ECX = ile razy powielić
              xlatb
              fast_stosb                ; powiel bajt
            $endif
            dec edx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        cmp [word ptr esi],0ffffh
      $enddo jne                        ; następna linia
    $else                               ; sprajt z obcinaniem
      $do
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount ebx                  ; EBX = ilość bloków

          mov edx,[toskip]              ; EDX = -(ile nie rysować)
          $do                           ; opuść EDX pikseli
            getcount ecx                ; ECX = ile opuścić
            add edx,ecx
            $if ns                      ; już na ekranie?
              sub edx,ecx               ; cofnij się poza ekran
              neg edx                   ; EDX = ile pikseli do granicy ekranu
              add edi,edx               ; EDI = lewy brzeg ekranu
              sub ecx,edx               ; ile opuścić
              getcount eax              ; ile kopiować
              xchg ecx,eax              ; EAX = opuścić, ECX = kopiować
              $leave                    ; lewy brzeg obcięty, rysuj sprajta
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            test ecx,ecx                ; sprawdź rodzaj bloku
            $if ns                      ; jeśli (+) kopiowanie bajtów
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                add esi,edx             ; ESI = skąd kopiować
                sub ecx,edx             ; ECX = ile kopiować
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              add esi,ecx               ; opuść ECX pikseli w definicji
            $else                       ; jeśli (-) powielanie bajtów
              neg ecx                   ; ECX = ile razy powielić
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                sub ecx,edx             ; ECX = ile razy powielić
                $if z                   ; jeśli nic do powielenia
                  inc esi               ; opuść powielany kolor
                $endif
                neg ecx                 ; ECX = -(ile razy powielić)
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              inc esi                   ; opuść powielany kolor
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            dec ebx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok

          or ebx,ebx                    ; sprawdź ilość bloków
          $if nz                        ; jeśli są bloki do wyrysowania
            mov edx,[todraw]            ; EDX = ile rysować
            $do
              sub edx,eax ;cmp          ; wszystko narysowane?
              $leave le                 ; jeśli tak, następna linia linia
              add edi,eax               ; opuść EAX pikseli
              cmp ecx,0                 ; sprawdź rodzaj bloku
              $if g                     ; ECX > 0: kopiowanie bajtów
                sub edx,ecx ;cmp        ; wszystko narysowane?
                $if g                   ; nie
                  push ebx
                  mov ebx,[using]
                  fast_xlatesiedi
                  pop ebx
                $else                   ; tak
                  add edx,ecx           ; EDX = poprzednia wartość
                  mov ecx,edx           ; ECX = ile jeszcze do wyrysowania
                  push ebx
                  mov ebx,[using]
                  fast_xlatesiedi
                  pop ebx
                  $leave                ; następna linia
                $endif
              $else                     ; ECX <= 0: powielanie bajtu
                $if ne                  ; ECX == 0: odpuść sobie
                  lodsb                 ; kolor do powielenia
                  neg ecx               ; ECX = ile razy powielić
                  sub edx,ecx ;cmp      ; wszystko narysowane?
                  $if g                 ; nie
                    push ebx
                    mov ebx,[using]
                    xlatb
                    pop ebx
                    fast_stosb          ; powiel bajt
                  $else                 ; tak
                    add edx,ecx         ; EDX = poprzednia wartość
                    mov ecx,edx         ; ECX = ile jeszcze do wyrysowania
                    push ebx
                    mov ebx,[using]
                    xlatb
                    pop ebx
                    fast_stosb          ; powiel tylko tyle razy
                    $leave              ; następna linia
                  $endif
                $endif
              $endif
              getcount ecx              ; ile opuścić
              getcount eax              ; ile kopiować
              xchg eax,ecx              ; EAX = opuścić, ECX = kopiować
              dec ebx                   ; zmniejsz ilość bloków
            $enddo jnz                  ; następny blok
          $endif
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        dec [rle.sy]
      $enddo jnz                        ; następna linia
    $endif
  $endif

; **** rób narzędzie kształtem sprajta

  cmp [doit],3
  $if e
    cmp ecx,4                           ; ECX = ile krawędzi jest na ekranie
    $if e                               ; cały sprajt (4 krawędzie)
      mov ebx,[using]
      $do                               ; rysuj [rle.sy] linii
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount edx                  ; EDX = ilość bloków
          $do                           ; rysuj [bx] bloków
            getcount eax                ; AEX = ile opuścić
            add edi,eax                 ; opuść EAX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            cmp ecx,0                   ; sprawdź rodzaj bloku
            $if g                       ; blok do skopiowania
              add esi,ecx
              fast_xlatedi
            $else                       ; blok do powielenia
              lodsb                     ; kolor do powielenia
              neg ecx                   ; ECX = ile razy powielić
              fast_xlatedi
            $endif
            dec edx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        cmp [word ptr esi],0ffffh
      $enddo jne                        ; następna linia
    $else                               ; sprajt z obcinaniem
      $do
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount ebx                  ; EBX = ilość bloków

          mov edx,[toskip]              ; EDX = -(ile nie rysować)
          $do                           ; opuść EDX pikseli
            getcount ecx                ; ECX = ile opuścić
            add edx,ecx
            $if ns                      ; już na ekranie?
              sub edx,ecx               ; cofnij się poza ekran
              neg edx                   ; EDX = ile pikseli do granicy ekranu
              add edi,edx               ; EDI = lewy brzeg ekranu
              sub ecx,edx               ; ile opuścić
              getcount eax              ; ile kopiować
              xchg ecx,eax              ; EAX = opuścić, ECX = kopiować
              $leave                    ; lewy brzeg obcięty, rysuj sprajta
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            test ecx,ecx                ; sprawdź rodzaj bloku
            $if ns                      ; jeśli (+) kopiowanie bajtów
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                add esi,edx             ; ESI = skąd kopiować
                sub ecx,edx             ; ECX = ile kopiować
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              add esi,ecx               ; opuść ECX pikseli w definicji
            $else                       ; jeśli (-) powielanie bajtów
              neg ecx                   ; ECX = ile razy powielić
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                sub ecx,edx             ; ECX = ile razy powielić
                $if z                   ; jeśli nic do powielenia
                  inc esi               ; opuść powielany kolor
                $endif
                neg ecx                 ; ECX = -(ile razy powielić)
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              inc esi                   ; opuść powielany kolor
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            dec ebx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok

          or ebx,ebx                    ; sprawdź ilość bloków
          $if nz                        ; jeśli są bloki do wyrysowania
            mov edx,[todraw]            ; EDX = ile rysować
            $do
              sub edx,eax ;cmp          ; wszystko narysowane?
              $leave le                 ; jeśli tak, następna linia linia
              add edi,eax               ; opuść EAX pikseli
              cmp ecx,0                 ; sprawdź rodzaj bloku
              $if g                     ; ECX > 0: kopiowanie bajtów
                sub edx,ecx ;cmp        ; wszystko narysowane?
                $if g                   ; nie
                  push ebx
                  mov ebx,[using]
                  add esi,ecx
                  fast_xlatedi
                  pop ebx
                $else                   ; tak
                  add edx,ecx           ; EDX = poprzednia wartość
                  mov ecx,edx           ; ECX = ile jeszcze do wyrysowania
                  push ebx
                  mov ebx,[using]
                  add esi,ecx
                  fast_xlatedi
                  pop ebx
                  $leave                ; następna linia
                $endif
              $else                     ; ECX <= 0: powielanie bajtu
                $if ne                  ; ECX == 0: odpuść sobie
                  lodsb                 ; kolor do powielenia
                  neg ecx               ; ECX = ile razy powielić
                  sub edx,ecx ;cmp      ; wszystko narysowane?
                  $if g                 ; nie
                    push ebx
                    mov ebx,[using]
                    fast_xlatedi
                    pop ebx
                  $else                 ; tak
                    add edx,ecx         ; EDX = poprzednia wartość
                    mov ecx,edx         ; ECX = ile jeszcze do wyrysowania
                    push ebx
                    mov ebx,[using]
                    fast_xlatedi
                    pop ebx
                    $leave              ; następna linia
                  $endif
                $endif
              $endif
              getcount ecx              ; ile opuścić
              getcount eax              ; ile kopiować
              xchg eax,ecx              ; EAX = opuścić, ECX = kopiować
              dec ebx                   ; zmniejsz ilość bloków
            $enddo jnz                  ; następny blok
          $endif
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        dec [rle.sy]
      $enddo jnz                        ; następna linia
    $endif
  $endif

; **** kopiuj ekran kształtem sprajta

  cmp [doit],4
  $if e
    mov eax,[rle.scr_adr]
    mov ebx,[using]
    sub ebx,eax
    mov [using],ebx

    cmp ecx,4                           ; ECX = ile krawędzi jest na ekranie
    $if e                               ; cały sprajt (4 krawędzie)
      $do                               ; rysuj [rle.sy] linii
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount ebx                  ; EBX = ilość bloków
          $do                           ; rysuj [bx] bloków
            getcount eax                ; AEX = ile opuścić
            add edi,eax                 ; opuść EAX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            cmp ecx,0                   ; sprawdź rodzaj bloku
            $if g                       ; blok do skopiowania
              add esi,ecx
              push esi
              mov esi,edi
              add esi,[using]
              fast_movsb                ; kopiuj bajty
              pop esi
            $else                       ; blok do powielenia
              lodsb                     ; kolor do powielenia
              neg ecx                   ; ECX = ile razy powielić
              push esi
              mov esi,edi
              add esi,[using]
              fast_movsb                ; kopiuj bajty
              pop esi
            $endif
            dec ebx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        cmp [word ptr esi],0ffffh
      $enddo jne                        ; następna linia
    $else                               ; sprajt z obcinaniem
      $do
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount ebx                  ; EBX = ilość bloków

          mov edx,[toskip]              ; EDX = -(ile nie rysować)
          $do                           ; opuść EDX pikseli
            getcount ecx                ; ECX = ile opuścić
            add edx,ecx
            $if ns                      ; już na ekranie?
              sub edx,ecx               ; cofnij się poza ekran
              neg edx                   ; EDX = ile pikseli do granicy ekranu
              add edi,edx               ; EDI = lewy brzeg ekranu
              sub ecx,edx               ; ile opuścić
              getcount eax              ; ile kopiować
              xchg ecx,eax              ; EAX = opuścić, ECX = kopiować
              $leave                    ; lewy brzeg obcięty, rysuj sprajta
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            test ecx,ecx                ; sprawdź rodzaj bloku
            $if ns                      ; jeśli (+) kopiowanie bajtów
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                add esi,edx             ; ESI = skąd kopiować
                sub ecx,edx             ; ECX = ile kopiować
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              add esi,ecx               ; opuść ECX pikseli w definicji
            $else                       ; jeśli (-) powielanie bajtów
              neg ecx                   ; ECX = ile razy powielić
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                sub ecx,edx             ; ECX = ile razy powielić
                $if z                   ; jeśli nic do powielenia
                  inc esi               ; opuść powielany kolor
                $endif
                neg ecx                 ; ECX = -(ile razy powielić)
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              inc esi                   ; opuść powielany kolor
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            dec ebx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok

          or ebx,ebx                    ; sprawdź ilość bloków
          $if nz                        ; jeśli są bloki do wyrysowania
            mov edx,[todraw]            ; EDX = ile rysować
            $do
              sub edx,eax ;cmp          ; wszystko narysowane?
              $leave le                 ; jeśli tak, następna linia linia
              add edi,eax               ; opuść EAX pikseli
              cmp ecx,0                 ; sprawdź rodzaj bloku
              $if g                     ; ECX > 0: kopiowanie bajtów
                sub edx,ecx ;cmp        ; wszystko narysowane?
                $if g                   ; nie
                  add esi,ecx
                  push esi
                  mov esi,edi
                  add esi,[using]
                  fast_movsb            ; kopiuj bajty
                  pop esi
                $else                   ; tak
                  add edx,ecx           ; EDX = poprzednia wartość
                  mov ecx,edx           ; ECX = ile jeszcze do wyrysowania
                  add esi,ecx
                  push esi
                  mov esi,edi
                  add esi,[using]
                  fast_movsb            ; kopiuj bajty
                  pop esi
                  $leave                ; następna linia
                $endif
              $else                     ; ECX <= 0: powielanie bajtu
                $if ne                  ; ECX == 0: odpuść sobie
                  lodsb                 ; kolor do powielenia
                  neg ecx               ; ECX = ile razy powielić
                  sub edx,ecx ;cmp      ; wszystko narysowane?
                  $if g                 ; nie
                    push esi
                    mov esi,edi
                    add esi,[using]
                    fast_movsb          ; kopiuj bajty
                    pop esi
                  $else                 ; tak
                    add edx,ecx         ; EDX = poprzednia wartość
                    mov ecx,edx         ; ECX = ile jeszcze do wyrysowania
                    push esi
                    mov esi,edi
                    add esi,[using]
                    fast_movsb          ; kopiuj bajty
                    pop esi
                    $leave              ; następna linia
                  $endif
                $endif
              $endif
              getcount ecx              ; ile opuścić
              getcount eax              ; ile kopiować
              xchg eax,ecx              ; EAX = opuścić, ECX = kopiować
              dec ebx                   ; zmniejsz ilość bloków
            $enddo jnz                  ; następny blok
          $endif
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        dec [rle.sy]
      $enddo jnz                        ; następna linia
    $endif
  $endif

; **** rysuj sprajta przezroczystego 1

  cmp [doit],5
  $if e
    cmp ecx,4                           ; ECX = ile krawędzi jest na ekranie
    $if e                               ; cały sprajt (4 krawędzie)
      mov ebx,[using]
      $do                               ; rysuj [rle.sy] linii
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount edx                  ; EDX = ilość bloków
          $do                           ; rysuj [bx] bloków
            getcount eax                ; AEX = ile opuścić
            add edi,eax                 ; opuść EAX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            cmp ecx,0                   ; sprawdź rodzaj bloku
            $if g                       ; blok do skopiowania
              push ecx
              dec ecx
              xor eax,eax
              $do
                mov ah,[esi+ecx]
                mov al,[edi+ecx]
                mov al,[ebx+eax]
                mov [edi+ecx],al
                dec ecx
              $enddo jns
              pop ecx
              add esi,ecx
              add edi,ecx
            $else                       ; blok do powielenia
              neg ecx                   ; ECX = ile razy powielić
              push ecx
              dec ecx
              xor eax,eax
              $do
                mov ah,[esi]
                mov al,[edi+ecx]
                mov al,[ebx+eax]
                mov [edi+ecx],al
                dec ecx
              $enddo jns
              pop ecx
              add edi,ecx
              inc esi
            $endif
            dec edx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        cmp [word ptr esi],0ffffh
      $enddo jne                        ; następna linia
    $else                               ; sprajt z obcinaniem
      $do
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount ebx                  ; EBX = ilość bloków

          mov edx,[toskip]              ; EDX = -(ile nie rysować)
          $do                           ; opuść EDX pikseli
            getcount ecx                ; ECX = ile opuścić
            add edx,ecx
            $if ns                      ; już na ekranie?
              sub edx,ecx               ; cofnij się poza ekran
              neg edx                   ; EDX = ile pikseli do granicy ekranu
              add edi,edx               ; EDI = lewy brzeg ekranu
              sub ecx,edx               ; ile opuścić
              getcount eax              ; ile kopiować
              xchg ecx,eax              ; EAX = opuścić, ECX = kopiować
              $leave                    ; lewy brzeg obcięty, rysuj sprajta
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            test ecx,ecx                ; sprawdź rodzaj bloku
            $if ns                      ; jeśli (+) kopiowanie bajtów
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                add esi,edx             ; ESI = skąd kopiować
                sub ecx,edx             ; ECX = ile kopiować
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              add esi,ecx               ; opuść ECX pikseli w definicji
            $else                       ; jeśli (-) powielanie bajtów
              neg ecx                   ; ECX = ile razy powielić
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                sub ecx,edx             ; ECX = ile razy powielić
                $if z                   ; jeśli nic do powielenia
                  inc esi               ; opuść powielany kolor
                $endif
                neg ecx                 ; ECX = -(ile razy powielić)
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              inc esi                   ; opuść powielany kolor
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            dec ebx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok

          or ebx,ebx                    ; sprawdź ilość bloków
          $if nz                        ; jeśli są bloki do wyrysowania
            mov edx,[todraw]            ; EDX = ile rysować
            $do
              sub edx,eax ;cmp          ; wszystko narysowane?
              $leave le                 ; jeśli tak, następna linia linia
              add edi,eax               ; opuść EAX pikseli
              cmp ecx,0                 ; sprawdź rodzaj bloku
              $if g                     ; ECX > 0: kopiowanie bajtów
                sub edx,ecx ;cmp        ; wszystko narysowane?
                $if g                   ; nie
                  push ebx
                  mov ebx,[using]
                  push ecx
                  dec ecx
                  xor eax,eax
                  $do
                    mov ah,[esi+ecx]
                    mov al,[edi+ecx]
                    mov al,[ebx+eax]
                    mov [edi+ecx],al
                    dec ecx
                  $enddo jns
                  pop ecx
                  add esi,ecx
                  add edi,ecx
                  pop ebx
                $else                   ; tak
                  add edx,ecx           ; EDX = poprzednia wartość
                  mov ecx,edx           ; ECX = ile jeszcze do wyrysowania
                  push ebx
                  mov ebx,[using]
                  push ecx
                  dec ecx
                  xor eax,eax
                  $do
                    mov ah,[esi+ecx]
                    mov al,[edi+ecx]
                    mov al,[ebx+eax]
                    mov [edi+ecx],al
                    dec ecx
                  $enddo jns
                  pop ecx
                  add esi,ecx
                  add edi,ecx
                  pop ebx
                  $leave                ; następna linia
                $endif
              $else                     ; ECX <= 0: powielanie bajtu
                $if ne                  ; ECX == 0: odpuść sobie
                  neg ecx               ; ECX = ile razy powielić
                  sub edx,ecx ;cmp      ; wszystko narysowane?
                  $if g                 ; nie
                    push ebx
                    mov ebx,[using]
                    push ecx
                    dec ecx
                    xor eax,eax
                    $do
                      mov ah,[esi]
                      mov al,[edi+ecx]
                      mov al,[ebx+eax]
                      mov [edi+ecx],al
                      dec ecx
                    $enddo jns
                    pop ecx
                    add edi,ecx
                    inc esi
                    pop ebx
                  $else                 ; tak
                    add edx,ecx         ; EDX = poprzednia wartość
                    mov ecx,edx         ; ECX = ile jeszcze do wyrysowania
                    push ebx
                    mov ebx,[using]
                    push ecx
                    dec ecx
                    xor eax,eax
                    $do
                      mov ah,[esi]
                      mov al,[edi+ecx]
                      mov al,[ebx+eax]
                      mov [edi+ecx],al
                      dec ecx
                    $enddo jns
                    pop ecx
                    add edi,ecx
                    inc esi
                    pop ebx
                    $leave              ; następna linia
                  $endif
                $endif
              $endif
              getcount ecx              ; ile opuścić
              getcount eax              ; ile kopiować
              xchg eax,ecx              ; EAX = opuścić, ECX = kopiować
              dec ebx                   ; zmniejsz ilość bloków
            $enddo jnz                  ; następny blok
          $endif
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        dec [rle.sy]
      $enddo jnz                        ; następna linia
    $endif
  $endif

; **** rysuj sprajta przezroczystego 2

  cmp [doit],6
  $if e
    cmp ecx,4                           ; ECX = ile krawędzi jest na ekranie
    $if e                               ; cały sprajt (4 krawędzie)
      mov ebx,[using]
      $do                               ; rysuj [rle.sy] linii
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount edx                  ; EDX = ilość bloków
          $do                           ; rysuj [bx] bloków
            getcount eax                ; AEX = ile opuścić
            add edi,eax                 ; opuść EAX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            cmp ecx,0                   ; sprawdź rodzaj bloku
            $if g                       ; blok do skopiowania
              push ecx
              dec ecx
              xor eax,eax
              $do
                mov al,[esi+ecx]
                mov ah,[edi+ecx]
                mov al,[ebx+eax]
                mov [edi+ecx],al
                dec ecx
              $enddo jns
              pop ecx
              add esi,ecx
              add edi,ecx
            $else                       ; blok do powielenia
              neg ecx                   ; ECX = ile razy powielić
              push ecx
              dec ecx
              xor eax,eax
              $do
                mov al,[esi]
                mov ah,[edi+ecx]
                mov al,[ebx+eax]
                mov [edi+ecx],al
                dec ecx
              $enddo jns
              pop ecx
              add edi,ecx
              inc esi
            $endif
            dec edx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        cmp [word ptr esi],0ffffh
      $enddo jne                        ; następna linia
    $else                               ; sprajt z obcinaniem
      $do
        push esi                        ; zapamiętaj pointer do sprajta
        push edi                        ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount ebx                  ; EBX = ilość bloków

          mov edx,[toskip]              ; EDX = -(ile nie rysować)
          $do                           ; opuść EDX pikseli
            getcount ecx                ; ECX = ile opuścić
            add edx,ecx
            $if ns                      ; już na ekranie?
              sub edx,ecx               ; cofnij się poza ekran
              neg edx                   ; EDX = ile pikseli do granicy ekranu
              add edi,edx               ; EDI = lewy brzeg ekranu
              sub ecx,edx               ; ile opuścić
              getcount eax              ; ile kopiować
              xchg ecx,eax              ; EAX = opuścić, ECX = kopiować
              $leave                    ; lewy brzeg obcięty, rysuj sprajta
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            test ecx,ecx                ; sprawdź rodzaj bloku
            $if ns                      ; jeśli (+) kopiowanie bajtów
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                add esi,edx             ; ESI = skąd kopiować
                sub ecx,edx             ; ECX = ile kopiować
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              add esi,ecx               ; opuść ECX pikseli w definicji
            $else                       ; jeśli (-) powielanie bajtów
              neg ecx                   ; ECX = ile razy powielić
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                sub ecx,edx             ; ECX = ile razy powielić
                $if z                   ; jeśli nic do powielenia
                  inc esi               ; opuść powielany kolor
                $endif
                neg ecx                 ; ECX = -(ile razy powielić)
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              inc esi                   ; opuść powielany kolor
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            dec ebx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok

          or ebx,ebx                    ; sprawdź ilość bloków
          $if nz                        ; jeśli są bloki do wyrysowania
            mov edx,[todraw]            ; EDX = ile rysować
            $do
              sub edx,eax ;cmp          ; wszystko narysowane?
              $leave le                 ; jeśli tak, następna linia linia
              add edi,eax               ; opuść EAX pikseli
              cmp ecx,0                 ; sprawdź rodzaj bloku
              $if g                     ; ECX > 0: kopiowanie bajtów
                sub edx,ecx ;cmp        ; wszystko narysowane?
                $if g                   ; nie
                  push ebx
                  mov ebx,[using]
                  push ecx
                  dec ecx
                  xor eax,eax
                  $do
                    mov al,[esi+ecx]
                    mov ah,[edi+ecx]
                    mov al,[ebx+eax]
                    mov [edi+ecx],al
                    dec ecx
                  $enddo jns
                  pop ecx
                  add esi,ecx
                  add edi,ecx
                  pop ebx
                $else                   ; tak
                  add edx,ecx           ; EDX = poprzednia wartość
                  mov ecx,edx           ; ECX = ile jeszcze do wyrysowania
                  push ebx
                  mov ebx,[using]
                  push ecx
                  dec ecx
                  xor eax,eax
                  $do
                    mov al,[esi+ecx]
                    mov ah,[edi+ecx]
                    mov al,[ebx+eax]
                    mov [edi+ecx],al
                    dec ecx
                  $enddo jns
                  pop ecx
                  add esi,ecx
                  add edi,ecx
                  pop ebx
                  $leave                ; następna linia
                $endif
              $else                     ; ECX <= 0: powielanie bajtu
                $if ne                  ; ECX == 0: odpuść sobie
                  neg ecx               ; ECX = ile razy powielić
                  sub edx,ecx ;cmp      ; wszystko narysowane?
                  $if g                 ; nie
                    push ebx
                    mov ebx,[using]
                    push ecx
                    dec ecx
                    xor eax,eax
                    $do
                      mov al,[esi]
                      mov ah,[edi+ecx]
                      mov al,[ebx+eax]
                      mov [edi+ecx],al
                      dec ecx
                    $enddo jns
                    pop ecx
                    add edi,ecx
                    inc esi
                    pop ebx
                  $else                 ; tak
                    add edx,ecx         ; EDX = poprzednia wartość
                    mov ecx,edx         ; ECX = ile jeszcze do wyrysowania
                    push ebx
                    mov ebx,[using]
                    push ecx
                    dec ecx
                    xor eax,eax
                    $do
                      mov al,[esi]
                      mov ah,[edi+ecx]
                      mov al,[ebx+eax]
                      mov [edi+ecx],al
                      dec ecx
                    $enddo jns
                    pop ecx
                    add edi,ecx
                    inc esi
                    pop ebx
                    $leave              ; następna linia
                  $endif
                $endif
              $endif
              getcount ecx              ; ile opuścić
              getcount eax              ; ile kopiować
              xchg eax,ecx              ; EAX = opuścić, ECX = kopiować
              dec ebx                   ; zmniejsz ilość bloków
            $enddo jnz                  ; następny blok
          $endif
        $endif
        pop edi                         ; EDI = pointer do ekranu
        pop esi                         ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        dec [rle.sy]
      $enddo jnz                        ; następna linia
    $endif
  $endif

; **** badaj kolizje

  cmp [doit],7
  $if e
    mov eax,0ffffffffh
    push eax
    cmp ecx,4                           ; ECX = ile krawędzi jest na ekranie
    $if e                               ; cały sprajt (4 krawędzie)
      lea ebx,[cc_temp]
      $do                               ; rysuj [rle.sy] linii
        mov [esi_sav],esi               ; zapamiętaj pointer do sprajta
        mov [edi_sav],edi               ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount edx                  ; EDX = ilość bloków
          $do                           ; rysuj [bx] bloków
            getcount eax                ; AEX = ile opuścić
            add edi,eax                 ; opuść EAX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            cmp ecx,0                   ; sprawdź rodzaj bloku
            $if g                       ; blok do skopiowania
              mov [ecx_sav],ecx
              dec ecx
              xor eax,eax
              $do
                mov al,[edi+ecx]
                cmp [ebx+eax],ah
                $if e
                  inc [BYTE PTR ebx+eax]
                  push eax
                $endif
                dec ecx
              $enddo jns
              mov ecx,[ecx_sav]
              add esi,ecx
              add edi,ecx
            $else                       ; blok do powielenia
              neg ecx                   ; ECX = ile razy powielić
              mov [ecx_sav],ecx
              dec ecx
              xor eax,eax
              $do
                mov al,[edi+ecx]
                cmp [ebx+eax],ah
                $if e
                  inc [BYTE PTR ebx+eax]
                  push eax
                $endif
                dec ecx
              $enddo jns
              mov ecx,[ecx_sav]
              add edi,ecx
              inc esi
            $endif
            dec edx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok
        $endif
        mov esi,[esi_sav]               ; EDI = pointer do ekranu
        mov edi,[edi_sav]               ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        cmp [word ptr esi],0ffffh
      $enddo jne                        ; następna linia
    $else                               ; sprajt z obcinaniem
      $do
        mov [esi_sav],esi               ; zapamiętaj pointer do sprajta
        mov [edi_sav],edi               ; zapamiętaj pointer do ekranu
        getcount eax                    ; EAX = długość linii definicji
        or eax,eax                      ; sprawdź długość linii
        $if nz                          ; jeśli linia coś zawiera
          getcount ebx                  ; EBX = ilość bloków

          mov edx,[toskip]              ; EDX = -(ile nie rysować)
          $do                           ; opuść EDX pikseli
            getcount ecx                ; ECX = ile opuścić
            add edx,ecx
            $if ns                      ; już na ekranie?
              sub edx,ecx               ; cofnij się poza ekran
              neg edx                   ; EDX = ile pikseli do granicy ekranu
              add edi,edx               ; EDI = lewy brzeg ekranu
              sub ecx,edx               ; ile opuścić
              getcount eax              ; ile kopiować
              xchg ecx,eax              ; EAX = opuścić, ECX = kopiować
              $leave                    ; lewy brzeg obcięty, rysuj sprajta
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            getcount ecx                ; ECX = (+/-) ile zapisać
            test ecx,ecx                ; sprawdź rodzaj bloku
            $if ns                      ; jeśli (+) kopiowanie bajtów
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                add esi,edx             ; ESI = skąd kopiować
                sub ecx,edx             ; ECX = ile kopiować
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              add esi,ecx               ; opuść ECX pikseli w definicji
            $else                       ; jeśli (-) powielanie bajtów
              neg ecx                   ; ECX = ile razy powielić
              add edx,ecx
              $if ns                    ; już na ekranie
                sub edx,ecx             ; cofnij się poza ekran
                neg edx                 ; EDX = ile pikseli do granicy ekranu
                add edi,edx             ; EDI = lewy brzeg ekranu
                sub ecx,edx             ; ECX = ile razy powielić
                $if z                   ; jeśli nic do powielenia
                  inc esi               ; opuść powielany kolor
                $endif
                neg ecx                 ; ECX = -(ile razy powielić)
                xor eax,eax             ; EAX = ile opuścić = 0
                $leave                  ; lewy brzeg obcięty, rysuj sprajta
              $endif
              inc esi                   ; opuść powielany kolor
            $endif
            add edi,ecx                 ; nie rysuj ECX pikseli
            dec ebx                     ; zmniejsz ilość bloków
          $enddo jnz                    ; następny blok

          or ebx,ebx                    ; sprawdź ilość bloków
          $if nz                        ; jeśli są bloki do wyrysowania
            mov edx,[todraw]            ; EDX = ile rysować
            $do
              sub edx,eax ;cmp          ; wszystko narysowane?
              $leave le                 ; jeśli tak, następna linia linia
              add edi,eax               ; opuść EAX pikseli
              cmp ecx,0                 ; sprawdź rodzaj bloku
              $if g                     ; ECX > 0: kopiowanie bajtów
                sub edx,ecx ;cmp        ; wszystko narysowane?
                $if g                   ; nie
                  mov [ebx_sav],ebx
                  lea ebx,[cc_temp]
                  mov [ecx_sav],ecx
                  dec ecx
                  xor eax,eax
                  $do
                    mov al,[edi+ecx]
                    cmp [ebx+eax],ah
                    $if e
                      inc [BYTE PTR ebx+eax]
                      push eax
                    $endif
                    dec ecx
                  $enddo jns
                  mov ecx,[ecx_sav]
                  mov ebx,[ebx_sav]
                  add esi,ecx
                  add edi,ecx
                $else                   ; tak
                  add edx,ecx           ; EDX = poprzednia wartość
                  mov ecx,edx           ; ECX = ile jeszcze do wyrysowania
                  mov [ebx_sav],ebx
                  lea ebx,[cc_temp]
                  mov [ecx_sav],ecx
                  dec ecx
                  xor eax,eax
                  $do
                    mov al,[edi+ecx]
                    cmp [ebx+eax],ah
                    $if e
                      inc [BYTE PTR ebx+eax]
                      push eax
                    $endif
                    dec ecx
                  $enddo jns
                  mov ecx,[ecx_sav]
                  mov ebx,[ebx_sav]
                  add esi,ecx
                  add edi,ecx
                  add esi,ecx
                  add edi,ecx
                  $leave                ; następna linia
                $endif
              $else                     ; ECX <= 0: powielanie bajtu
                $if ne                  ; ECX == 0: odpuść sobie
                  neg ecx               ; ECX = ile razy powielić
                  sub edx,ecx ;cmp      ; wszystko narysowane?
                  $if g                 ; nie
                    mov [ebx_sav],ebx
                    lea ebx,[cc_temp]
                    mov [ecx_sav],ecx
                    dec ecx
                    xor eax,eax
                    $do
                      mov al,[edi+ecx]
                      cmp [ebx+eax],ah
                      $if e
                        inc [BYTE PTR ebx+eax]
                        push eax
                      $endif
                      dec ecx
                    $enddo jns
                    mov ecx,[ecx_sav]
                    mov ebx,[ebx_sav]
                    add edi,ecx
                    inc esi
                  $else                 ; tak
                    add edx,ecx         ; EDX = poprzednia wartość
                    mov ecx,edx         ; ECX = ile jeszcze do wyrysowania
                    mov [ebx_sav],ebx
                    lea ebx,[cc_temp]
                    mov [ecx_sav],ecx
                    dec ecx
                    xor eax,eax
                    $do
                      mov al,[edi+ecx]
                      cmp [ebx+eax],ah
                      $if e
                        inc [BYTE PTR ebx+eax]
                        push eax
                      $endif
                      dec ecx
                    $enddo jns
                    mov ecx,[ecx_sav]
                    mov ebx,[ebx_sav]
                    add edi,ecx
                    inc esi
                    $leave              ; następna linia
                  $endif
                $endif
              $endif
              getcount ecx              ; ile opuścić
              getcount eax              ; ile kopiować
              xchg eax,ecx              ; EAX = opuścić, ECX = kopiować
              dec ebx                   ; zmniejsz ilość bloków
            $enddo jnz                  ; następny blok
          $endif
        $endif
        mov esi,[esi_sav]               ; EDI = pointer do ekranu
        mov edi,[edi_sav]               ; ESI = pointer do definicji sprajta
        getcount eax                    ; EAX = długość linii definicji
        add esi,eax                     ; następna linia sprajta
        test [rle.mirror],02h
        $if z
          add edi,[llen]                ; następna linia na ekranie
        $else
          sub edi,[llen]                ; następna linia na ekranie
        $endif
        dec [rle.sy]
      $enddo jnz                        ; następna linia
    $endif

    mov ebx,[cc_result]
    lea edx,[cc_temp]
    xor ecx,ecx
    $do
      pop eax
      cmp eax,0ffffffffh
      $leave e
      test eax,[using]
      $if nz
        mov [ebx],al
        inc ebx
      $endif
      mov [edx+eax],cl
    $enddo
    mov [BYTE PTR ebx],cl

  $endif

@@endproc:

  pop edi
  pop esi
  pop ebp
  pop edx
  pop ecx
  pop ebx
  pop eax

ENDPROC

PROCEDURE       _pconv
ARG             def :PTR BYTE, tool :PTR BYTE
LOCAL           size :DWORD

  push eax
  push ebx
  push ecx
  push edx
  push ebp
  push esi
  push edi

  xor eax,eax
  mov [size],eax
  cld
  mov esi,[def]
  mov ebx,[tool]
  $do
    push esi
    getcount eax
    or eax,eax
    $if nz
      getcount edx
      $do
        getcount eax
        getcount ecx
        mov edi,esi
        cmp ecx,0
        $if g
          fast_xlatesiedi
        $else
          lodsb
          xlatb
          stosb
        $endif
        dec edx
      $enddo jnz
    $endif
    pop esi
    getcount eax
    add esi,eax
    add [size],eax
    cmp [word ptr esi],0ffffh
  $enddo jne

@@endproc:

  pop edi
  pop esi
  pop ebp
  pop edx
  pop ecx
  pop ebx
  pop eax
  mov eax,[size]

ENDPROC

PROCEDURE       _ptouch
ARG             def :PTR BYTE
LOCAL           size :DWORD

  push eax
  push ebx
  push ecx
  push edx
  push ebp
  push esi
  push edi

  xor eax,eax
  mov [size],eax
  cld
  mov esi,[def]
  $do
    push esi
    getcount eax
    or eax,eax
    $if nz
      getcount edx
      $do
        getcount eax
        getcount ecx
        mov edi,esi
        cmp ecx,0
        $if g
          fast_movsb
        $else
          lodsb
          stosb
        $endif
        dec edx
      $enddo jnz
    $endif
    pop esi
    getcount eax
    add esi,eax
    add [size],eax
    cmp [word ptr esi],0ffffh
  $enddo jne

@@endproc:

  pop edi
  pop esi
  pop ebp
  pop edx
  pop ecx
  pop ebx
  pop eax
  mov eax,[size]

ENDPROC

PROCEDURE       _pcsum
ARG             def :PTR BYTE
LOCAL           csum :DWORD

  push eax
  push ebx
  push ecx
  push edx
  push ebp
  push esi
  push edi

  xor eax,eax
  mov [csum],eax
  cld
  mov esi,[def]
  $do
    push esi
    getcount eax
    or eax,eax
    $if nz
      mov ecx,eax
      xor eax,eax
      $do
        lodsb
        add [csum],eax
      $enddo loop
    $endif
    pop esi
    getcount eax
    add esi,eax
    add [csum],eax
    cmp [word ptr esi],0ffffh
  $enddo jne

@@endproc:

  pop edi
  pop esi
  pop ebp
  pop edx
  pop ecx
  pop ebx
  pop eax
  mov eax,[csum]

ENDPROC

PROCEDURE       _fast_xlat
ARG             dst :PTR BYTE, conv :PTR BYTE, len :DWORD

  push eax
  push ebx
  push ecx
  push edi

  mov edi,[dst]
  mov ebx,[conv]
  mov ecx,[len]
  fast_xlatedi

  pop edi
  pop ecx
  pop ebx
  pop eax

ENDPROC

END