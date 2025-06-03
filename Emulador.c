#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tipos
typedef unsigned char byte;         // 8 bits
typedef unsigned int palavra;       // 32 bits
typedef unsigned long int microinstrucao; // 64 bits, usamos apenas 36 bits

// Registradores
palavra MAR = 0, MDR = 0, PC = 0;   // Acesso à memória
byte MBR = 0;                       // Acesso à memória

palavra SP = 0, LV = 0, TOS = 0,    // Operação da ULA
        OPC = 0, CPP = 0, H = 0;    // Operação da ULA

microinstrucao MIR;                 // Contém a Microinstrução Atual
palavra MPC = 0;                    // Contém o endereço para a próxima Microinstrução

// Barramentos
palavra Barramento_B, Barramento_C;

// Flip-Flops
byte N, Z;

// Auxiliares para Decodificar Microinstrução
byte MIR_B, MIR_Operacao, MIR_Deslocador, MIR_MEM, MIR_pulo;
palavra MIR_C;

// Armazenamento de Controle
microinstrucao Armazenamento[512];

// Memória Principal
byte Memoria[100000000];

// Protótipos das Funções
void carregar_microprograma_de_controle();
void carregar_programa(const char *programa);
void exibir_processos();
void decodificar_microinstrucao();
void atribuir_barramento_B();
void realizar_operacao_ALU();
void atribuir_barramento_C();
void operar_memoria();
void pular();
void binario(void* valor, int tipo);

// Laço Principal
int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Erro: Informe o arquivo do programa como argumento.\n");
        return EXIT_FAILURE;
    }

    carregar_microprograma_de_controle();
    carregar_programa(argv[1]);

    while (1) {
        exibir_processos();
        MIR = Armazenamento[MPC];

        decodificar_microinstrucao();
        atribuir_barramento_B();
        realizar_operacao_ALU();
        atribuir_barramento_C();
        operar_memoria();
        pular();
    }

    return 0;
}

// Implementação das Funções

// Carrega o microprograma de controle
void carregar_microprograma_de_controle() {
    FILE* microPrograma = fopen("microprog.rom", "rb");

    if (!microPrograma) {
        fprintf(stderr, "Erro: Não foi possível abrir 'microprog.rom'.\n");
        exit(EXIT_FAILURE);
    }

    fread(Armazenamento, sizeof(microinstrucao), 512, microPrograma);
    fclose(microPrograma);
}

// Carrega o programa principal na memória
void carregar_programa(const char* prog) {
    FILE* programa = fopen(prog, "rb");
    if (!programa) {
        fprintf(stderr, "Erro: Não foi possível abrir o programa '%s'.\n", prog);
        exit(EXIT_FAILURE);
    }

    palavra tamanho;
    byte tamanho_temp[4];
    fread(tamanho_temp, sizeof(byte), 4, programa);
    memcpy(&tamanho, tamanho_temp, 4);

    fread(Memoria, sizeof(byte), 20, programa);
    fread(&Memoria[0x0401], sizeof(byte), tamanho - 20, programa);

    fclose(programa);
}

// Decodifica a microinstrução atual
void decodificar_microinstrucao() {
    MIR_B = (MIR) & 0b1111;
    MIR_MEM = (MIR >> 4) & 0b111;
    MIR_C = (MIR >> 7) & 0b111111111;
    MIR_Operacao = (MIR >> 16) & 0b111111;
    MIR_Deslocador = (MIR >> 22) & 0b11;
    MIR_pulo = (MIR >> 24) & 0b111;
    MPC = (MIR >> 27) & 0b111111111;
}

// Define o valor do Barramento B
void atribuir_barramento_B() {
    switch (MIR_B) {
        case 0: Barramento_B = MDR; break;
        case 1: Barramento_B = PC; break;
        case 2: 
            Barramento_B = MBR;
            if (MBR & 0b10000000) {
                Barramento_B |= (0b111111111111111111111111 << 8);
            }
            break;
        case 3: Barramento_B = MBR; break;
        case 4: Barramento_B = SP; break;
        case 5: Barramento_B = LV; break;
        case 6: Barramento_B = CPP; break;
        case 7: Barramento_B = TOS; break;
        case 8: Barramento_B = OPC; break;
        default: Barramento_B = 0; break;
    }
}

// Realiza a operação da ULA
void realizar_operacao_ALU() {
    switch (MIR_Operacao) {
        case 12: Barramento_C = H & Barramento_B; break;
        case 17: Barramento_C = 1; break;
        case 18: Barramento_C = -1; break;
        case 20: Barramento_C = Barramento_B; break;
        case 24: Barramento_C = H; break;
        case 26: Barramento_C = ~H; break;
        case 28: Barramento_C = H | Barramento_B; break;
        case 44: Barramento_C = ~Barramento_B; break;
        case 53: Barramento_C = Barramento_B + 1; break;
        case 54: Barramento_C = Barramento_B - 1; break;
        case 57: Barramento_C = H + 1; break;
        case 59: Barramento_C = -H; break;
        case 60: Barramento_C = H + Barramento_B; break;
        case 61: Barramento_C = H + Barramento_B + 1; break;
        case 63: Barramento_C = Barramento_B - H; break;
        default: Barramento_C = 0; break;
    }

    N = (Barramento_C == 0) ? 1 : 0;
    Z = !N;

    switch (MIR_Deslocador) {
        case 1: Barramento_C <<= 8; break;
        case 2: Barramento_C >>= 1; break;
    }
}

// Atribui o valor de C aos registradores
void atribuir_barramento_C() {
    if (MIR_C & 0b000000001) MAR = Barramento_C;
    if (MIR_C & 0b000000010) MDR = Barramento_C;
    if (MIR_C & 0b000000100) PC = Barramento_C;
    if (MIR_C & 0b000001000) SP = Barramento_C;
    if (MIR_C & 0b000010000) LV = Barramento_C;
    if (MIR_C & 0b000100000) CPP = Barramento_C;
    if (MIR_C & 0b001000000) TOS = Barramento_C;
    if (MIR_C & 0b010000000) OPC = Barramento_C;
    if (MIR_C & 0b100000000) H = Barramento_C;
}

// Operações de memória
void operar_memoria() {
    if (MIR_MEM & 0b001) MBR = Memoria[PC];
    if (MIR_MEM & 0b010) memcpy(&MDR, &Memoria[MAR * 4], 4);
    if (MIR_MEM & 0b100) memcpy(&Memoria[MAR * 4], &MDR, 4);
}

// Controle de fluxo
void pular() {
    if (MIR_pulo & 0b001) MPC |= (N << 8);
    if (MIR_pulo & 0b010) MPC |= (Z << 8);
    if (MIR_pulo & 0b100) MPC |= MBR;
}

// Exibe os processos em execução
void exibir_processos() {
    if (LV && SP) {
        printf("\t\t  PILHA DE OPERANDOS\n");
        printf("========================================\n");
        printf("     END\t   BINÁRIO DO VALOR\tVALOR\n");

        for (int i = SP; i >= LV; i--) {
            palavra valor;
            memcpy(&valor, &Memoria[i * 4], 4);

            if (i == SP) printf("SP ->");
            else if (i == LV) printf("LV ->");
            else printf("     ");

            printf("%X ", i);
            binario(&valor, 1);
            printf(" %d\n", valor);
        }
        printf("========================================\n");
    }

    if (PC >= 0x0401) {
        printf("\n\t\t\tÁrea do Programa\n");
        printf("========================================\n");
        printf("\t\tBinário\t HEX  END DE BYTE\n");
        for (int i = PC - 2; i <= PC + 3; i++) {
            if (i == PC) printf("Em execução >>  ");
            else printf("\t\t");
            binario(&Memoria[i], 2);
            printf(" 0x%02X \t%X\n", Memoria[i], i);
        }
        printf("========================================\n\n");
    }

    printf("\t\tREGISTRADORES\n");
    printf("\tBINÁRIO\t\t\t\tHEX\n");
    printf("MAR: "); binario(&MAR, 3); printf("\t%x\n", MAR);
    printf("MDR: "); binario(&MDR, 3); printf("\t%x\n", MDR);
    printf("PC:  "); binario(&PC, 3); printf("\t%x\n", PC);
    printf("MBR: \t\t"); binario(&MBR, 2); printf("\t\t%x\n", MBR);
    printf("SP:  "); binario(&SP, 3); printf("\t%x\n", SP);
    printf("LV:  "); binario(&LV, 3); printf("\t%x\n", LV);
    printf("CPP: "); binario(&CPP, 3); printf("\t%x\n", CPP);
    printf("TOS: "); binario(&TOS, 3); printf("\t%x\n", TOS);
    printf("OPC: "); binario(&OPC, 3); printf("\t%x\n", OPC);
    printf("H:   "); binario(&H, 3); printf("\t%x\n", H);
    printf("MPC: \t\t"); binario(&MPC, 5); printf("\t%x\n", MPC);
    printf("MIR: "); binario(&MIR, 4); printf("\n");

    getchar();
}

// Imprime valores em binário conforme o tipo
void binario(void* valor, int tipo) {
    switch (tipo) {
        case 1: {
            byte* v = (byte*) valor;
            for (int i = 3; i >= 0; i--) {
                byte aux = *(v + i);
                for (int j = 0; j < 8; j++) {
                    printf("%d", (aux >> 7) & 1);
                    aux <<= 1;
                }
                printf(" ");
            }
            break;
        }
        case 2: {
            byte aux = *((byte*) valor);
            for (int j = 0; j < 8; j++) {
                printf("%d", (aux >> 7) & 1);
                aux <<= 1;
            }
            break;
        }
        case 3: {
            palavra aux = *((palavra*) valor);
            for (int j = 0; j < 32; j++) {
                printf("%d", (aux >> 31) & 1);
                aux <<= 1;
            }
            break;
        }
        case 4: {
            microinstrucao aux = *((microinstrucao*) valor);
            for (int j = 0; j < 36; j++) {
                if (j == 9 || j == 12 || j == 20 || j == 29 || j == 32) printf(" ");
                printf("%ld", (aux >> 35) & 1);
                aux <<= 1;
            }
            break;
        }
        case 5: {
            palavra aux = *((palavra*) valor) << 23;
            for (int j = 0; j < 9; j++) {
                printf("%d", (aux >> 31) & 1);
                aux <<= 1;
            }
            break;
        }
    }
}
