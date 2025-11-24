#ifndef ISR_H
#define ISR_H

#include <stddef.h>
#include <stdint.h>


#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10

typedef struct 
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_res;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t int_no;
	uint32_t err_code;

    //stack frame
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed)) int_stack_t;


void isr_default();
void isr_divide_by_zero();
void isr_keyboard();

/* São apenas protótipos. Referem-se às rotinas que foram todas implementadas em assembly,
no arquivo isr_low.s */

void isr_stub_0(void);
void isr_stub_1(void);
void isr_stub_2(void);
void isr_stub_3(void);
void isr_stub_4(void);
void isr_stub_5(void);
void isr_stub_6(void);
void isr_stub_7(void);
void isr_stub_8(void);
void isr_stub_9(void);

void isr_stub_10(void);
void isr_stub_11(void);
void isr_stub_12(void);
void isr_stub_13(void);
void isr_stub_14(void);
void isr_stub_15(void);
void isr_stub_16(void);
void isr_stub_17(void);
void isr_stub_18(void);
void isr_stub_19(void);

void isr_stub_20(void);
void isr_stub_21(void);
void isr_stub_22(void);
void isr_stub_23(void);
void isr_stub_24(void);
void isr_stub_25(void);
void isr_stub_26(void);
void isr_stub_27(void);
void isr_stub_28(void);
void isr_stub_29(void);

void isr_stub_30(void);
void isr_stub_31(void);
void isr_stub_32(void);
void isr_stub_33(void);
void isr_stub_34(void);
void isr_stub_35(void);
void isr_stub_36(void);
void isr_stub_37(void);
void isr_stub_38(void);
void isr_stub_39(void);

void isr_stub_40(void);
void isr_stub_41(void);
void isr_stub_42(void);
void isr_stub_43(void);
void isr_stub_44(void);
void isr_stub_45(void);
void isr_stub_46(void);
void isr_stub_47(void);
void isr_stub_48(void);
void isr_stub_49(void);

void isr_stub_50(void);
void isr_stub_51(void);
void isr_stub_52(void);
void isr_stub_53(void);
void isr_stub_54(void);
void isr_stub_55(void);
void isr_stub_56(void);
void isr_stub_57(void);
void isr_stub_58(void);
void isr_stub_59(void);

void isr_stub_60(void);
void isr_stub_61(void);
void isr_stub_62(void);
void isr_stub_63(void);
void isr_stub_64(void);
void isr_stub_65(void);
void isr_stub_66(void);
void isr_stub_67(void);
void isr_stub_68(void);
void isr_stub_69(void);

void isr_stub_70(void);
void isr_stub_71(void);
void isr_stub_72(void);
void isr_stub_73(void);
void isr_stub_74(void);
void isr_stub_75(void);
void isr_stub_76(void);
void isr_stub_77(void);
void isr_stub_78(void);
void isr_stub_79(void);

void isr_stub_80(void);
void isr_stub_81(void);
void isr_stub_82(void);
void isr_stub_83(void);
void isr_stub_84(void);
void isr_stub_85(void);
void isr_stub_86(void);
void isr_stub_87(void);
void isr_stub_88(void);
void isr_stub_89(void);

void isr_stub_90(void);
void isr_stub_91(void);
void isr_stub_92(void);
void isr_stub_93(void);
void isr_stub_94(void);
void isr_stub_95(void);
void isr_stub_96(void);
void isr_stub_97(void);
void isr_stub_98(void);
void isr_stub_99(void);

void isr_stub_100(void);
void isr_stub_101(void);
void isr_stub_102(void);
void isr_stub_103(void);
void isr_stub_104(void);
void isr_stub_105(void);
void isr_stub_106(void);
void isr_stub_107(void);
void isr_stub_108(void);
void isr_stub_109(void);

void isr_stub_110(void);
void isr_stub_111(void);
void isr_stub_112(void);
void isr_stub_113(void);
void isr_stub_114(void);
void isr_stub_115(void);
void isr_stub_116(void);
void isr_stub_117(void);
void isr_stub_118(void);
void isr_stub_119(void);

void isr_stub_120(void);
void isr_stub_121(void);
void isr_stub_122(void);
void isr_stub_123(void);
void isr_stub_124(void);
void isr_stub_125(void);
void isr_stub_126(void);
void isr_stub_127(void);
void isr_stub_128(void);
void isr_stub_129(void);

void isr_stub_130(void);
void isr_stub_131(void);
void isr_stub_132(void);
void isr_stub_133(void);
void isr_stub_134(void);
void isr_stub_135(void);
void isr_stub_136(void);
void isr_stub_137(void);
void isr_stub_138(void);
void isr_stub_139(void);

void isr_stub_140(void);
void isr_stub_141(void);
void isr_stub_142(void);
void isr_stub_143(void);
void isr_stub_144(void);
void isr_stub_145(void);
void isr_stub_146(void);
void isr_stub_147(void);
void isr_stub_148(void);
void isr_stub_149(void);

void isr_stub_150(void);
void isr_stub_151(void);
void isr_stub_152(void);
void isr_stub_153(void);
void isr_stub_154(void);
void isr_stub_155(void);
void isr_stub_156(void);
void isr_stub_157(void);
void isr_stub_158(void);
void isr_stub_159(void);

void isr_stub_160(void);
void isr_stub_161(void);
void isr_stub_162(void);
void isr_stub_163(void);
void isr_stub_164(void);
void isr_stub_165(void);
void isr_stub_166(void);
void isr_stub_167(void);
void isr_stub_168(void);
void isr_stub_169(void);

void isr_stub_170(void);
void isr_stub_171(void);
void isr_stub_172(void);
void isr_stub_173(void);
void isr_stub_174(void);
void isr_stub_175(void);
void isr_stub_176(void);
void isr_stub_177(void);
void isr_stub_178(void);
void isr_stub_179(void);

void isr_stub_180(void);
void isr_stub_181(void);
void isr_stub_182(void);
void isr_stub_183(void);
void isr_stub_184(void);
void isr_stub_185(void);
void isr_stub_186(void);
void isr_stub_187(void);
void isr_stub_188(void);
void isr_stub_189(void);

void isr_stub_190(void);
void isr_stub_191(void);
void isr_stub_192(void);
void isr_stub_193(void);
void isr_stub_194(void);
void isr_stub_195(void);
void isr_stub_196(void);
void isr_stub_197(void);
void isr_stub_198(void);
void isr_stub_199(void);

void isr_stub_200(void);
void isr_stub_201(void);
void isr_stub_202(void);
void isr_stub_203(void);
void isr_stub_204(void);
void isr_stub_205(void);
void isr_stub_206(void);
void isr_stub_207(void);
void isr_stub_208(void);
void isr_stub_209(void);

void isr_stub_210(void);
void isr_stub_211(void);
void isr_stub_212(void);
void isr_stub_213(void);
void isr_stub_214(void);
void isr_stub_215(void);
void isr_stub_216(void);
void isr_stub_217(void);
void isr_stub_218(void);
void isr_stub_219(void);

void isr_stub_220(void);
void isr_stub_221(void);
void isr_stub_222(void);
void isr_stub_223(void);
void isr_stub_224(void);
void isr_stub_225(void);
void isr_stub_226(void);
void isr_stub_227(void);
void isr_stub_228(void);
void isr_stub_229(void);

void isr_stub_230(void);
void isr_stub_231(void);
void isr_stub_232(void);
void isr_stub_233(void);
void isr_stub_234(void);
void isr_stub_235(void);
void isr_stub_236(void);
void isr_stub_237(void);
void isr_stub_238(void);
void isr_stub_239(void);

void isr_stub_240(void);
void isr_stub_241(void);
void isr_stub_242(void);
void isr_stub_243(void);
void isr_stub_244(void);
void isr_stub_245(void);
void isr_stub_246(void);
void isr_stub_247(void);
void isr_stub_248(void);
void isr_stub_249(void);

void isr_stub_250(void);
void isr_stub_251(void);
void isr_stub_252(void);
void isr_stub_253(void);
void isr_stub_254(void);
void isr_stub_255(void);

#endif