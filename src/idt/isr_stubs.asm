;/*--------------------------------------------------------------------------
;*  File name:  isr_stubs.asm
;*  Author:  Aldenor Sombra de Oliveira
;*  Data de criação: 01-08-2020
;*--------------------------------------------------------------------------
;Este arquivo possui as rotinas para a manipulação de interrupções 
;--------------------------------------------------------------------------*/
; Interrupt descriptor table


section .text
; section .data
;align 16
;global idt64, pointer ;idt64.pointer

global isr_common
isr_common:	
	
	cli
    pushad

    mov eax, esp ; Salvo o topo da stack em RAX - apenas um teste.	
    push eax    ; passa como argumento

    extern isr_global_handler
    call isr_global_handler

    add esp, 4  ; limpa argumento da pilha

    popad
    add esp, 8 ; erroNum e intNum	
    sti
    iret

;********************************************************************
; Funções handlers
;********************************************************************
extern isr_save_context
[global isr_stub_0]
isr_stub_0:
 	push dword 0
	push dword 0
 jmp isr_common
 

[global isr_stub_1]
isr_stub_1:
	push dword 0
	push dword 1
 jmp isr_common
 

[global isr_stub_2]
isr_stub_2:
	push dword 0
	push dword 2
 jmp isr_common 

[global isr_stub_3]
isr_stub_3: 
 	push dword 0
	push dword 3
 jmp isr_common
 

[global isr_stub_4]
isr_stub_4:
	push dword 0
	push dword 4
 jmp isr_common
 

[global isr_stub_5]
isr_stub_5:
	push dword 0
	push dword 5
 jmp isr_common
 

[global isr_stub_6]
isr_stub_6:
	push dword 0
	push dword 6
 jmp isr_common
 
[global isr_stub_7]
isr_stub_7:
	push dword 0
	push dword 7
 jmp isr_common
 
[global isr_stub_8]
isr_stub_8:
	push dword 8
 jmp isr_common
 
[global isr_stub_9]
isr_stub_9:
	push dword 0
	push dword 9
 jmp isr_common
 
[global isr_stub_10]
isr_stub_10:
	push dword 10
 jmp isr_common
 
[global isr_stub_11]
isr_stub_11:
	push dword 11
 jmp isr_common
 
[global isr_stub_12]
isr_stub_12:
	push dword 12
 jmp isr_common
 
[global isr_stub_13]
isr_stub_13: 
	push dword 13
 jmp isr_common
 
[global isr_stub_14]
isr_stub_14:
	push dword 14
 jmp isr_common
 
[global isr_stub_15]
isr_stub_15:
	push dword 0
	push dword 15
 jmp isr_common
 
[global isr_stub_16]
isr_stub_16: 
 	push dword 0
	push dword 16
 jmp isr_common
 
[global isr_stub_17]
isr_stub_17:
 	push dword 17
 jmp isr_common
 
[global isr_stub_18]
isr_stub_18:
 	push dword 0
	push dword 18
 jmp isr_common
 
[global isr_stub_19]
isr_stub_19:
	push dword 0
	push dword 19
 jmp isr_common
 
[global isr_stub_20]
isr_stub_20:
	push dword 0
	push dword 20
 jmp isr_common

[global isr_stub_21]
isr_stub_21:
	push dword 0
	push dword 21
 jmp isr_common

[global isr_stub_22]
isr_stub_22:
	push dword 0
	push dword 22
 jmp isr_common

[global isr_stub_23]
isr_stub_23:
 	push dword 0
	push dword 23
 jmp isr_common

[global isr_stub_24]
isr_stub_24: 
	push dword 0
	push dword 24
 jmp isr_common 

[global isr_stub_25]
isr_stub_25:
	push dword 0
	push dword 25
 jmp isr_common

[global isr_stub_26]
isr_stub_26:
	push dword 0
	push dword 26
 jmp isr_common

[global isr_stub_27]
isr_stub_27:
	push dword 0
	push dword 27
 jmp isr_common

[global isr_stub_28]
isr_stub_28:
	push dword 0
	push dword 28
 jmp isr_common

[global isr_stub_29]
isr_stub_29:
	push dword 0
	push dword 29
 jmp isr_common

[global isr_stub_30]
isr_stub_30:	
	push dword 30
 jmp isr_common

[global isr_stub_31]
isr_stub_31:
	push dword 0
	push dword 31
 jmp isr_common

[global isr_stub_32]
isr_stub_32:
 push dword 0
 push dword 32 
jmp isr_common

 

[global isr_stub_33]
isr_stub_33:
	push dword 0
	push dword 33	
 jmp isr_common

[global isr_stub_34]
isr_stub_34:
	push dword 0
	push dword 34
 jmp isr_common 

[global isr_stub_35]
isr_stub_35:
	push dword 0
	push dword 35
 jmp isr_common 

[global isr_stub_36]
isr_stub_36:
	push dword 0
	push dword 36
 jmp isr_common 

[global isr_stub_37]
isr_stub_37:
	push dword 0
	push dword 37
 jmp isr_common 

[global isr_stub_38]
isr_stub_38:
	push dword 0
	push dword 38
 jmp isr_common 

[global isr_stub_39]
isr_stub_39:
	push dword 0
	push dword 39
 jmp isr_common 

[global isr_stub_40]
isr_stub_40:
	push dword 0
	push dword 40
 jmp isr_common 
 

[global isr_stub_41]
isr_stub_41:
	push dword 0
	push dword 41
 jmp isr_common 

[global isr_stub_42]
isr_stub_42:
	push dword 0
	push dword 42
 jmp isr_common 

[global isr_stub_43]
isr_stub_43:
	push dword 0
	push dword 43
 jmp isr_common 

[global isr_stub_44]
isr_stub_44:
	push dword 0
	push dword 44
 jmp isr_common 

[global isr_stub_45]
isr_stub_45:
	push dword 0
	push dword 45
 jmp isr_common 

[global isr_stub_46]
isr_stub_46:
	push dword 0
	push dword 46
 jmp isr_common 

[global isr_stub_47]
isr_stub_47:
	push dword 0
	push dword 47
 jmp isr_common 


[global isr_stub_48]
isr_stub_48:
	push dword 0
 	push dword 48
 jmp isr_common
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[global isr_stub_49]
isr_stub_49:
push dword 0
 push dword 49
 jmp isr_common

[global isr_stub_50]
isr_stub_50:
push dword 0
 push dword 50
 jmp isr_common

[global isr_stub_51]
isr_stub_51:
push dword 0
 push dword 51
 jmp isr_common


[global isr_stub_52]
isr_stub_52:
push dword 0
 push dword 52
 jmp isr_common

[global isr_stub_53]
isr_stub_53:
push dword 0
 push dword 53
 jmp isr_common

[global isr_stub_54]
isr_stub_54:
push dword 0
 push dword 54
 jmp isr_common

[global isr_stub_55]
isr_stub_55:
push dword 0
 push dword 55
 jmp isr_common

[global isr_stub_56]
isr_stub_56:
push dword 0
 push dword 56
 jmp isr_common

[global isr_stub_57]
isr_stub_57:
push dword 0
 push dword 57
 jmp isr_common

[global isr_stub_58]
isr_stub_58:
push dword 0
 push dword 58
 jmp isr_common

[global isr_stub_59]
isr_stub_59:
push dword 0
 push dword 59
 jmp isr_common

[global isr_stub_60]
isr_stub_60:
push dword 0
 push dword 60
jmp isr_common
;jmp __switch_context

[global isr_stub_61]

isr_stub_61:
push dword 0
 push dword 61
 jmp isr_common

[global isr_stub_62]
isr_stub_62:
push dword 0
 push dword 62
 jmp isr_common

[global isr_stub_63]
isr_stub_63:
push dword 0
 push dword 63
 jmp isr_common

[global isr_stub_64]
isr_stub_64:
push dword 0
 push dword 64
 jmp isr_common

[global isr_stub_65]
isr_stub_65:
push dword 0
 push dword 65
 jmp isr_common

[global isr_stub_66]
isr_stub_66:
push dword 0
 push dword 66
 jmp isr_common

[global isr_stub_67]
isr_stub_67:
push dword 0
 push dword 67
 jmp isr_common

[global isr_stub_68]
isr_stub_68:
push dword 0
 push dword 68
 jmp isr_common

[global isr_stub_69]
isr_stub_69:
push dword 0
 push dword 69
 jmp isr_common

[global isr_stub_70]
isr_stub_70:
push dword 0
 push dword 70
;jmp isr_save_context
jmp isr_common

[global isr_stub_71]
isr_stub_71:
push dword 0
 push dword 71
 jmp isr_common

[global isr_stub_72]
isr_stub_72:
push dword 0
 push dword 72
 jmp isr_common

[global isr_stub_73]
isr_stub_73:
push dword 0
 push dword 73
 jmp isr_common

[global isr_stub_74]
isr_stub_74:
push dword 0
 push dword 74
 jmp isr_common

[global isr_stub_75]
isr_stub_75:
push dword 0
 push dword 75
 jmp isr_common

[global isr_stub_76]

isr_stub_76:
push dword 0
 push dword 76
 jmp isr_common

[global isr_stub_77]
isr_stub_77:
push dword 0
 push dword 77
 jmp isr_common

[global isr_stub_78]
isr_stub_78:
push dword 0
 push dword 78
 jmp isr_common

[global isr_stub_79]
isr_stub_79:
push dword 0
 push dword 79
 jmp isr_common

[global isr_stub_80]
isr_stub_80:
push dword 0
 push dword 80
 jmp isr_common

[global isr_stub_81]
isr_stub_81:
push dword 0
 push dword 81
 jmp isr_common

[global isr_stub_82]
isr_stub_82:
push dword 0
 push dword 82
 jmp isr_common

[global isr_stub_83]
isr_stub_83:
push dword 0
 push dword 83
 jmp isr_common

[global isr_stub_84]
isr_stub_84:
push dword 0
 push dword 84
 jmp isr_common

[global isr_stub_85]
isr_stub_85:
push dword 0
 push dword 85
 jmp isr_common

[global isr_stub_86]
isr_stub_86:
push dword 0
 push dword 86
 jmp isr_common

[global isr_stub_87]
isr_stub_87:
push dword 0
 push dword 87
 jmp isr_common

[global isr_stub_88]
isr_stub_88:
push dword 0
 push dword 88
 jmp isr_common

[global isr_stub_89]
isr_stub_89:
push dword 0
 push dword 89
 jmp isr_common

[global isr_stub_90]
isr_stub_90:
push dword 0
 push dword 90
 jmp isr_common

[global isr_stub_91]
isr_stub_91:
push dword 0
 push dword 91
 jmp isr_common

[global isr_stub_92]
isr_stub_92:
push dword 0
 push dword 92
 jmp isr_common

[global isr_stub_93]
isr_stub_93:
push dword 0
 push dword 93
 jmp isr_common

[global isr_stub_94]
isr_stub_94:
push dword 0
 push dword 94
 jmp isr_common

[global isr_stub_95]
isr_stub_95:
push dword 0
 push dword 95
 jmp isr_common

[global isr_stub_96]
isr_stub_96:
push dword 0
 push dword 96
 jmp isr_common

[global isr_stub_97]
isr_stub_97:
push dword 0
 push dword 97
 jmp isr_common

[global isr_stub_98]
isr_stub_98:
push dword 0
 push dword 98
 jmp isr_common

[global isr_stub_99]
isr_stub_99:
push dword 0
 push dword 99
 jmp isr_common

[global isr_stub_100]
isr_stub_100:
push dword 0
 push dword 100
 jmp isr_common

[global isr_stub_101]
isr_stub_101:
 push dword 101
 jmp isr_common

[global isr_stub_102]
isr_stub_102:
 push dword 102
 jmp isr_common

[global isr_stub_103]
isr_stub_103:
 push dword 103
 jmp isr_common

[global isr_stub_104]
isr_stub_104:
 push dword 104
 jmp isr_common

[global isr_stub_105]
isr_stub_105:
 push dword 105
 jmp isr_common

[global isr_stub_106]
isr_stub_106:
 push dword 106
 jmp isr_common

[global isr_stub_107]
isr_stub_107:
 push dword 107
 jmp isr_common

[global isr_stub_108]
isr_stub_108:
 push dword 108
 jmp isr_common

[global isr_stub_109]
isr_stub_109:
 push dword 109
 jmp isr_common

[global isr_stub_110]
isr_stub_110:
 push dword 110
 jmp isr_common

[global isr_stub_111]
isr_stub_111:
 push dword 111
 jmp isr_common

[global isr_stub_112]
isr_stub_112:
 push dword 112
 jmp isr_common

[global isr_stub_113]
isr_stub_113:
 push dword 113
 jmp isr_common

[global isr_stub_114]
isr_stub_114:
 push dword 114
 jmp isr_common

[global isr_stub_115]
isr_stub_115:
 push dword 115
 jmp isr_common

[global isr_stub_116]
isr_stub_116:
 push dword 116
 jmp isr_common

[global isr_stub_117]
isr_stub_117:
 push dword 117
 jmp isr_common

[global isr_stub_118]
isr_stub_118:
 push dword 118
 jmp isr_common

[global isr_stub_119]
isr_stub_119:
 push dword 119
 jmp isr_common

[global isr_stub_120]
isr_stub_120:
 push dword 120
 jmp isr_common

[global isr_stub_121]
isr_stub_121:
 push dword 121
 jmp isr_common

[global isr_stub_122]
isr_stub_122:
 push dword 122
 jmp isr_common

[global isr_stub_123]
isr_stub_123:
 push dword 123
 jmp isr_common

[global isr_stub_124]
isr_stub_124:
 push dword 124
 jmp isr_common

[global isr_stub_125]
isr_stub_125:
 push dword 125
 jmp isr_common

[global isr_stub_126]
isr_stub_126:
 ;push dword 126
 jmp isr_common

[global isr_stub_127]
isr_stub_127:
 push dword 127
 jmp isr_common

[global isr_stub_128]
isr_stub_128:
	push dword 0
	push dword 128
jmp isr_common


[global isr_stub_129]
isr_stub_129:
 push dword 129
 jmp isr_common

[global isr_stub_130]
isr_stub_130:
 push dword 130
 jmp isr_common

[global isr_stub_131]
isr_stub_131:
 push dword 131
 jmp isr_common

[global isr_stub_132]
isr_stub_132:
 push dword 132
 jmp isr_common

[global isr_stub_133]
isr_stub_133:
 push dword 133
 jmp isr_common

[global isr_stub_134]
isr_stub_134:
 push dword 134
 jmp isr_common

[global isr_stub_135]
isr_stub_135:
 push dword 135
 jmp isr_common

[global isr_stub_136]
isr_stub_136:
 push dword 136
 jmp isr_common

[global isr_stub_137]
isr_stub_137:
 push dword 137
 jmp isr_common

[global isr_stub_138]
isr_stub_138:
 push dword 138
 jmp isr_common

[global isr_stub_139]
isr_stub_139:
 push dword 139
 jmp isr_common

[global isr_stub_140]
isr_stub_140:
 push dword 140
 jmp isr_common

[global isr_stub_141]
isr_stub_141:
 push dword 141
 jmp isr_common

[global isr_stub_142]
isr_stub_142:
 push dword 142
 jmp isr_common

[global isr_stub_143]
isr_stub_143:
 push dword 143
 jmp isr_common

[global isr_stub_144]
isr_stub_144:
 push dword 144
 jmp isr_common

[global isr_stub_145]
isr_stub_145:
 push dword 145
 jmp isr_common

[global isr_stub_146]
isr_stub_146:
 push dword 146
 jmp isr_common

[global isr_stub_147]
isr_stub_147:
 push dword 147
 jmp isr_common

[global isr_stub_148]
isr_stub_148:
 push dword 148
 jmp isr_common

[global isr_stub_149]
isr_stub_149:
 push dword 149
 jmp isr_common

[global isr_stub_150]
isr_stub_150:
 push dword 150
 jmp isr_common

[global isr_stub_151]
isr_stub_151:
 push dword 151
 jmp isr_common

[global isr_stub_152]
isr_stub_152:
 push dword 152
 jmp isr_common

[global isr_stub_153]
isr_stub_153:
 push dword 153
 jmp isr_common

[global isr_stub_154]
isr_stub_154:
 push dword 154
 jmp isr_common

[global isr_stub_155]
isr_stub_155:
 push dword 155
 jmp isr_common

[global isr_stub_156]
isr_stub_156:
 push dword 156
 jmp isr_common

[global isr_stub_157]
isr_stub_157:
 push dword 157
 jmp isr_common

[global isr_stub_158]
isr_stub_158:
 push dword 158
 jmp isr_common

[global isr_stub_159]
isr_stub_159:
 push dword 159
 jmp isr_common

[global isr_stub_160]
isr_stub_160:
 push dword 160
 jmp isr_common

[global isr_stub_161]
isr_stub_161:
 push dword 161
 jmp isr_common

[global isr_stub_162]
isr_stub_162:
 push dword 162
 jmp isr_common

[global isr_stub_163]
isr_stub_163:
 push dword 163
 jmp isr_common

[global isr_stub_164]
isr_stub_164:
 push dword 164
 jmp isr_common

[global isr_stub_165]
isr_stub_165:
 push dword 165
 jmp isr_common
 
[global isr_stub_166]
isr_stub_166:
 push dword 166
 jmp isr_common

[global isr_stub_167]
isr_stub_167:
 push dword 167
 jmp isr_common

[global isr_stub_168]
isr_stub_168:
 push dword 168
 jmp isr_common

[global isr_stub_169]
isr_stub_169:
 push dword 169
 jmp isr_common

[global isr_stub_170]
isr_stub_170:
 push dword 170
 jmp isr_common

[global isr_stub_171]
isr_stub_171:
 push dword 171
 jmp isr_common

[global isr_stub_172]
isr_stub_172:
 push dword 172
 jmp isr_common

[global isr_stub_173]
isr_stub_173:
 push dword 173
 jmp isr_common

[global isr_stub_174]
isr_stub_174:
 push dword 174
 jmp isr_common

[global isr_stub_175]
isr_stub_175:
 push dword 175
 jmp isr_common

[global isr_stub_176]
isr_stub_176:
 push dword 176
 jmp isr_common

[global isr_stub_177]
isr_stub_177:
 push dword 177
 jmp isr_common

[global isr_stub_178]
isr_stub_178:
 push dword 178
 jmp isr_common

[global isr_stub_179]
isr_stub_179:
 push dword 179
 jmp isr_common

[global isr_stub_180]
isr_stub_180:
 push dword 180
 jmp isr_common

[global isr_stub_181]
isr_stub_181:
 push dword 181
 jmp isr_common

[global isr_stub_182]
isr_stub_182:
 push dword 182
 jmp isr_common

[global isr_stub_183]
isr_stub_183:
 push dword 183
 jmp isr_common

[global isr_stub_184]
isr_stub_184:
 push dword 184
 jmp isr_common

[global isr_stub_185]
isr_stub_185:
 push dword 185
 jmp isr_common

[global isr_stub_186]
isr_stub_186:
 push dword 186
 jmp isr_common

[global isr_stub_187]
isr_stub_187:
 push dword 187
 jmp isr_common

[global isr_stub_188]
isr_stub_188:
 push dword 188
 jmp isr_common

[global isr_stub_189]
isr_stub_189:
 push dword 189
 jmp isr_common

[global isr_stub_190]
isr_stub_190:
 push dword 190
 jmp isr_common

[global isr_stub_191]
isr_stub_191:
 push dword 191
 jmp isr_common

[global isr_stub_192]
isr_stub_192:
 push dword 192
 jmp isr_common

[global isr_stub_193]
isr_stub_193:
 push dword 193
 jmp isr_common

[global isr_stub_194]
isr_stub_194:
 push dword 194
 jmp isr_common

[global isr_stub_195]
isr_stub_195:
 push dword 195
 jmp isr_common

[global isr_stub_196]
isr_stub_196:
 push dword 196
 jmp isr_common

[global isr_stub_197]
isr_stub_197:
 push dword 197
 jmp isr_common

[global isr_stub_198]
isr_stub_198:
 push dword 198
 jmp isr_common

[global isr_stub_199]
isr_stub_199:
 push dword 199
 jmp isr_common

[global isr_stub_200]
isr_stub_200:
 push dword 200
 jmp isr_common

[global isr_stub_201]
isr_stub_201:
 push dword 201
 jmp isr_common

[global isr_stub_202]
isr_stub_202:
 push dword 202
 jmp isr_common

[global isr_stub_203]
isr_stub_203:
 push dword 203
 jmp isr_common

[global isr_stub_204]
isr_stub_204:
 push dword 204
 jmp isr_common

[global isr_stub_205]
isr_stub_205:
 push dword 205
 jmp isr_common

[global isr_stub_206]
isr_stub_206:
 push dword 206
 jmp isr_common

[global isr_stub_207]
isr_stub_207:
 push dword 207
 jmp isr_common

[global isr_stub_208]
isr_stub_208:
 push dword 208
 jmp isr_common

[global isr_stub_209]
isr_stub_209:
 push dword 209
 jmp isr_common

[global isr_stub_210]
isr_stub_210:
 push dword 210
 jmp isr_common

[global isr_stub_211]
isr_stub_211:
 push dword 211
 jmp isr_common

[global isr_stub_212]
isr_stub_212:
 push dword 212
 jmp isr_common

[global isr_stub_213]
isr_stub_213:
 push dword 213
 jmp isr_common

[global isr_stub_214]
isr_stub_214:
 push dword 214
 jmp isr_common

[global isr_stub_215]
isr_stub_215:
 push dword 215
 jmp isr_common

[global isr_stub_216]
isr_stub_216:
 push dword 216
 jmp isr_common

[global isr_stub_217]
isr_stub_217:
 push dword 217
 jmp isr_common

[global isr_stub_218]
isr_stub_218:
 push dword 218
 jmp isr_common

[global isr_stub_219]
isr_stub_219:
 push dword 219
 jmp isr_common

[global isr_stub_220]
isr_stub_220:
 push dword 220
 jmp isr_common

[global isr_stub_221]
isr_stub_221:
 push dword 221
 jmp isr_common

[global isr_stub_222]
isr_stub_222:
 push dword 222
 jmp isr_common

[global isr_stub_223]
isr_stub_223:
 push dword 223
 jmp isr_common

[global isr_stub_224]
isr_stub_224:
 push dword 224
 jmp isr_common

[global isr_stub_225]
isr_stub_225:
 push dword 225
 jmp isr_common

[global isr_stub_226]
isr_stub_226:
 push dword 226
 jmp isr_common

[global isr_stub_227]
isr_stub_227:
 push dword 227
 jmp isr_common

[global isr_stub_228]
isr_stub_228:
 push dword 228
 jmp isr_common

[global isr_stub_229]
isr_stub_229:
 push dword 229
 jmp isr_common

[global isr_stub_230]
isr_stub_230:
 push dword 230
 jmp isr_common

[global isr_stub_231]
isr_stub_231:
 push dword 231
 jmp isr_common

[global isr_stub_232]
isr_stub_232:
 push dword 232
 jmp isr_common

[global isr_stub_233]
isr_stub_233:
 push dword 233
 jmp isr_common

[global isr_stub_234]
isr_stub_234:
 push dword 234
 jmp isr_common

[global isr_stub_235]
isr_stub_235:
 push dword 235
 jmp isr_common

[global isr_stub_236]
isr_stub_236:
 push dword 236
 jmp isr_common

[global isr_stub_237]
isr_stub_237:
 push dword 237
 jmp isr_common

[global isr_stub_238]
isr_stub_238:
 push dword 238
 jmp isr_common

;--------------------------------------------------------
;Este handler é utilizado para as interrupções de tempo
;produzidas pelo APIC TIMER e utilizadas pelo scheduler.
;Passei a usar o IRQ_BASE==0xEF(239).
;--------------------------------------------------------
[global isr_stub_239]
isr_stub_239:
	push dword 0
 	push dword 239
 jmp isr_common
 ;--------------------------------------------------------

[global isr_stub_240]
isr_stub_240:
 push dword 240
 jmp isr_common

[global isr_stub_241]
isr_stub_241:
 push dword 241
 jmp isr_common

[global isr_stub_242]
isr_stub_242:
 push dword 242
 jmp isr_common

[global isr_stub_243]
isr_stub_243:
 push dword 243
 jmp isr_common

[global isr_stub_244]
isr_stub_244:
 push dword 244
 jmp isr_common

[global isr_stub_245]
isr_stub_245:
 push dword 245
 jmp isr_common

[global isr_stub_246]
isr_stub_246:
 push dword 246
 jmp isr_common

[global isr_stub_247]
isr_stub_247:
 push dword 247
 jmp isr_common

[global isr_stub_248]
isr_stub_248:
 push dword 248
 jmp isr_common

[global isr_stub_249]
isr_stub_249:
 push dword 249
 jmp isr_common

[global isr_stub_250]
isr_stub_250:
 push dword 250
 jmp isr_common

[global isr_stub_251]
isr_stub_251:
	push dword 0
	push dword 251 
 jmp isr_common

[global isr_stub_252]
isr_stub_252:
 push dword 252
 jmp isr_common

[global isr_stub_253]
isr_stub_253:
 push dword 253
 jmp isr_common

[global isr_stub_254]
isr_stub_254:
 push dword 0
	push dword 254 
 jmp isr_common

[global isr_stub_255]
isr_stub_255:
	push dword 0
	push dword 255 
 jmp isr_common