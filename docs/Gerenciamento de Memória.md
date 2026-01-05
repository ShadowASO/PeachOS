# Gerenciamento de Memória

### memory_setup()
    bootmem_init()
        Extrai e grava em variáveis estáticas, acessíveis por funções, os endereços físicos e virtual do kernel
        e o seu tamanho. Extrai também informações, com o e820, relacionadas à memória presente do sistema.
    
    boot_early_init(final do kernel, tamanho da memória alocável)
    
        ALOCADOR BOOT EARLY - É o alocador utilizado antes de concluirmos a
    configuração do kheap. Define o tamanho máximo de memória disponível 
    para uso/alocação pelo alocador provisório. Deve ser reservado espaço 
    suficiente para criar as estruturas iniciais. Após o kheap_init, 
    o boot_early NÃO PODE SER USADO para alocações de memória.
    
    pmm_calc_bitmap_size_bytes(tamanho da memória)
        
        Calcula o tamanho em byte do espaço necessário para acomodar um bitmap suficiente
        para controlar o tamanho de memória informada. Normalmente, o bitmap será criado
        para acomodar toda a memória identificada pelo e820.
    
    boot_early_kalloc(tamanho em bytes do bitmap, alinhamento em 4096)
    
        Faz a alocação de um bloco de memória usando o boot_early. Devolvendo o endereço 
        virtual mapped-identity do início do bloco de memória alocado e alinhado em "align"
    
    pmm_init(endereço do bitmap, tamanho da memória presente);
    
        Faz a inicialização do bitmap, marcando as regiões livre e indisponíveis, 
        além de calcular a memória física disponível, a quantidade de frames.
    
    paging_init_minimal(endereço da estrutura &ctx);
    
        paging_create_directory(ctx);
            Cria um page_directory para utilização nas rotinase de paging.
            Utiliza a função informada em ctx->alloc_page_aligned para obter
            um bloco de memória de 4096 byts(4K);
            
            paging_ctx_t ctx = {
                .alloc_page_aligned = early_alloc_wrapper,  //função que será usada para alocar memória
                .virt_to_phys       = 0, // identity no bootstrap
                .bootstrap_identity_limit = 64u * 1024u * 1024u, // 64MiB
                .kernel_virt_base   = 0xC0000000u,
                .kernel_phys_start  = k_phys_start,
                .kernel_phys_end    = k_phys_end,
                .kernel_page_flags  = PAGE_RW,
            };   
    
        paging_kmap_init(kernel_directory, ctx);
        
            Cria um "page_table" para utilização pelo kmap e o atribui ao page_directory.
            Esse page_table será usado para mapear o endereço  "0xFFC00000u".
            
            O kmap permite que se faça o mapeamento de memória dinamicamente, evitando 
            a necessidade de mapear toda a memória já no bootstrap.
            
        create_page_table(kernel_directory, ctx, phys, pde_flags, 0);
        
            Devolve o endereço físico do PT para o endereço informado. Caso não exista no PDE, 
            um novo PT é criado e atribuído ao PDE.
            
            Durante o bootstrap, é utilizado o ctx->alloc_page_aligned. Posteriormente, usa-se
            o pmm_alloc_frame();
            
    map_heap_initial()
    
        Faz o mapeamento inicial da área de memória definida para a heap.
        
        pmm_alloc_frame()
        
            Aloca um frame de memória e devolve o seu endereço físico.
        
        kmap()
        
            Mapeia em kmap o frame físico e faz a sua limpeza
            
        paging_map()
        
            Esta rotina é que faz o mapeamento do endereço virtual para o frame indicado 
            pelo endereço físico. Se houver a necessidade, é criada um "page table(PT)" 
            e vinculado ao page_directory.
        
            
        
        
        
