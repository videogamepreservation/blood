; Fire cellular frame loop
;

	IDEAL
        P486N

	MODEL	FLAT, C

	CODESEG

	EXTRN C CoolTable:BYTE

;	void CellularFrame(void * buffer, short count)

PROC    	C CellularFrame buffer:PTR, cols:DWORD, rows:DWORD
PUBLIC		CellularFrame

		USES	esi,edi,ebp

		mov	esi,[buffer]
		mov	edi,[cols]
		mov	eax,[rows]
		imul	edi
		mov	ebp,eax
		sub	eax,eax
		sub	ecx,ecx
		dec	esi

loopStart:
		lea	edx,[esi+edi]	; line + 1
		inc	esi
		mov	al,[edx]
		mov	cl,[edx+1]
		mov	ebx,eax
		add	ebx,ecx
		mov	al,[edx+2]

		add	edx,edi		; line + 2
		add	ebx,eax
		mov	cl,[edx+1]
		add	ebx,ecx

		cmp	cl,060h
		ja	hot

		mov	al,[OFFSET CoolTable+ebx]
		dec	ebp
		mov	[esi],al
		jz	@@exit

		lea	edx,[esi+edi]	; line + 1
		inc	esi
		mov	al,[edx]
		mov	cl,[edx+1]
		mov	ebx,eax
		add	ebx,ecx

		mov	al,[edx+2]
		add	edx,edi		; line + 2
		add	ebx,eax
		mov	cl,[edx+1]
		add	ebx,ecx

		cmp	cl,060h
		ja	hot

		mov	al,[OFFSET CoolTable+ebx]
		dec	ebp
		mov	[esi],al
		jz	@@exit


		lea	edx,[esi+edi]	; line + 1
		inc	esi
		mov	al,[edx]
		mov	cl,[edx+1]
		mov	ebx,eax
		add	ebx,ecx

		mov	al,[edx+2]
		add	edx,edi		; line + 2
		add	ebx,eax
		mov	cl,[edx+1]
		add	ebx,ecx

		cmp	cl,060h
		ja	hot

		mov	al,[OFFSET CoolTable+ebx]
		dec	ebp
		mov	[esi],al
		jnz	loopStart

@@exit:
		ret

hot:
		add	ebx,ecx

		mov	al,[edx]
		mov	cl,[edx+2]
		add	edx,edi		; line + 3
		add	ebx,eax
		add	ebx,ecx

		mov	al,[edx+1]
		add	ebx,eax
		shr	ebx,1

		mov	al,[OFFSET CoolTable+ebx]
		dec	ebp
		mov	[esi],al
		jz	@@exit

		lea	edx,[esi+edi]	; line + 1
		inc	esi
		mov	al,[edx]
		mov	cl,[edx+1]
		mov	ebx,eax
		add	ebx,ecx

		mov	al,[edx+2]
		add	edx,edi		; line + 2
		add	ebx,eax
		mov	cl,[edx+1]
		add	ebx,ecx

		cmp	cl,060h
		jna	@@cold
		add	ebx,ecx

		mov	al,[edx]
		mov	cl,[edx+2]
		add	edx,edi		; line + 3
		add	ebx,eax
		add	ebx,ecx

		mov	al,[edx+1]
		add	ebx,eax
		shr	ebx,1

		mov	al,[OFFSET CoolTable+ebx]
		dec	ebp
		mov	[esi],al
		jnz	loopStart
		ret


@@cold:
		mov	al,[OFFSET CoolTable+ebx]
		dec	ebp
		mov	[esi],al
		jnz	loopStart

		ret

ENDP	CellularFrame

END
