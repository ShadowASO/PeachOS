# -----------------------------------------------------------------------------
#             Em 18-11-2025: Versão 1.0.0                                    
# -----------------------------------------------------------------------------
a) Implementadas a tabela IDT  e o PIC;
b) Criados os stubs e a isr_common para tratar as interrupções e exceções;

# -----------------------------------------------------------------------------
#             Em 20-11-2025: Versão 1.0.0                                    
# -----------------------------------------------------------------------------
a) finalmente consegui fazer a virtualização do kernel com uso de high halj;

# -----------------------------------------------------------------------------
#             Em 21-11-2025: Versão 1.0.0                                    
# -----------------------------------------------------------------------------
01) Ajustes na estrutura do projeto;
02) Criação do gerenciador de memória física;
03) Ajustes na rotina do kernel.asm para corrigir o high half corretamente. Tive
que ajustar o linker.ld para criar a variável KERNEL_OFFSET que uso para loca-
licar o kernel no endereço físico correto e permtir que haja uma relação correta
0x00100000 -> 0xC0000000;
04) Criados as constantes de manipuladores de bits;
05) Criado um arquivo config.h para o kernel e um arquivo config.inc para o có-
digo assembly;

# -----------------------------------------------------------------------------
#             Em 24-11-2025: Versão 1.0.0                                    
# -----------------------------------------------------------------------------
a) criadas as rotinas de extração do mapa de memória durante o boot. Muito tra-
balho;
b) ajuste e refinamentos no código de boot e inicialização do kernel para a ex-
tração do mapa da meória;
c) criei repositório para o projeto do PeachOS;

# -----------------------------------------------------------------------------
#             Em 25-11-2025: Versão 1.0.0                                    
# -----------------------------------------------------------------------------
a) revisando o Kernel Development e fazendo ajustes no código do bootloader,
a partir da compreensão adquirida no modo real e seu endereçamento;

# -----------------------------------------------------------------------------
#             Em 23-12-2025: Versão 1.0.0                                    
# -----------------------------------------------------------------------------
a) concluída a implementação do filesystem e do driver do FAT16;
