#ifndef CPU_H
#define CPU_H

#include <stddef.h>
#include <stdint.h>

/* -------------------------------------------------------------------------
 *  Flags de registradores (x86) — EFLAGS, CR0
 *  Úteis na hora de montar/desmontar estados de CPU.
 * ------------------------------------------------------------------------- */

/* Bits em EFLAGS */
#define EFLAGS_CF_BIT      0   /* Carry Flag                */
#define EFLAGS_PF_BIT      2   /* Parity Flag               */
#define EFLAGS_AF_BIT      4   /* Auxiliary Carry Flag      */
#define EFLAGS_ZF_BIT      6   /* Zero Flag                 */
#define EFLAGS_SF_BIT      7   /* Sign Flag                 */
#define EFLAGS_TF_BIT      8   /* Trap Flag                 */
#define EFLAGS_IF_BIT      9   /* Interrupt Enable Flag     */
#define EFLAGS_DF_BIT      10  /* Direction Flag            */
#define EFLAGS_OF_BIT      11  /* Overflow Flag             */

/* Máscaras prontas dos bits de EFLAGS */
#define EFLAGS_CF          BIT(EFLAGS_CF_BIT)
#define EFLAGS_PF          BIT(EFLAGS_PF_BIT)
#define EFLAGS_AF          BIT(EFLAGS_AF_BIT)
#define EFLAGS_ZF          BIT(EFLAGS_ZF_BIT)
#define EFLAGS_SF          BIT(EFLAGS_SF_BIT)
#define EFLAGS_TF          BIT(EFLAGS_TF_BIT)
#define EFLAGS_IF          BIT(EFLAGS_IF_BIT)
#define EFLAGS_DF          BIT(EFLAGS_DF_BIT)
#define EFLAGS_OF          BIT(EFLAGS_OF_BIT)

/* Bits de controle em CR0 (subset principal) */
#define CR0_PE_BIT         0   /* Protected Mode Enable     */
#define CR0_MP_BIT         1
#define CR0_EM_BIT         2
#define CR0_TS_BIT         3
#define CR0_ET_BIT         4
#define CR0_NE_BIT         5
#define CR0_WP_BIT         16
#define CR0_AM_BIT         18
#define CR0_NW_BIT         29
#define CR0_CD_BIT         30
#define CR0_PG_BIT         31  /* Paging Enable             */

/* Máscaras prontas de CR0 */
#define CR0_PE             BIT(CR0_PE_BIT)
#define CR0_MP             BIT(CR0_MP_BIT)
#define CR0_EM             BIT(CR0_EM_BIT)
#define CR0_TS             BIT(CR0_TS_BIT)
#define CR0_ET             BIT(CR0_ET_BIT)
#define CR0_NE             BIT(CR0_NE_BIT)
#define CR0_WP             BIT(CR0_WP_BIT)
#define CR0_AM             BIT(CR0_AM_BIT)
#define CR0_NW             BIT(CR0_NW_BIT)
#define CR0_CD             BIT(CR0_CD_BIT)
#define CR0_PG             BIT(CR0_PG_BIT)

#endif