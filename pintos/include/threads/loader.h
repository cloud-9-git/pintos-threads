#ifndef THREADS_LOADER_H
#define THREADS_LOADER_H  // 이 파일이 이미 포함됨

/* Constants fixed by the PC BIOS. */
#define LOADER_BASE 0x7c00      /* Physical address of loader's base.(부트로더가 메모리에 올라오는 시작 물리 주소) */
#define LOADER_END  0x7e00      /* Physical address of end of loader.(부트로더가 끝나는 물리 주소) */

/* Physical address of kernel base. */
#define LOADER_KERN_BASE 0x8004000000  // 커널이 사용하는 높은 가상 주소 기준점

/* Kernel virtual address at which all physical memory is mapped. */
#define LOADER_PHYS_BASE 0x200000  // 커널 이미지가 실제 물리 메모리에서 적재되기 시작되는 주소

/* Multiboot infos */
#define MULTIBOOT_INFO       0x7000  // 멀티부터 정보가 저장되는 위치
#define MULTIBOOT_FLAG       MULTIBOOT_INFO  // 멀티부트 플래그 위치
#define MULTIBOOT_MMAP_LEN   MULTIBOOT_INFO + 44  // 매모리 맵 길이 정보
#define MULTIBOOT_MMAP_ADDR  MULTIBOOT_INFO + 48  // 메모리 맵 자체의 주소가 저장된 위치

#define E820_MAP MULTIBOOT_INFO + 52    // BIOS E820 메모리 맵 정보 위치
#define E820_MAP4 MULTIBOOT_INFO + 56   // E820 관련 추가 정보 위치

/* Important loader physical addresses. */
#define LOADER_SIG (LOADER_END - LOADER_SIG_LEN)   /* 0xaa55 BIOS signature.(부트 섹터 끝부분의 BIOS 시그니처가 저장되는 위치) */
#define LOADER_ARGS (LOADER_SIG - LOADER_ARGS_LEN)     /* Command-line args.(커널 커맨드라인 인자 문자열이 저장되는 영역 시작 위치) */
#define LOADER_ARG_CNT (LOADER_ARGS - LOADER_ARG_CNT_LEN) /* Number of args.(커맨드라인 인자의 개수가 저장되는 위치) */

/* Sizes of loader data structures. */
#define LOADER_SIG_LEN 2        // BIOS 시그니처 길이: 2바이트
#define LOADER_ARGS_LEN 128     // 커맨드라인 문자열 전체에 할당한 최대 길이
#define LOADER_ARG_CNT_LEN 4    // 인자 개수를 저장하는 공간 크기: uint32_t 하나 크기

/* GDT selectors defined by loader.
   More selectors are defined by userprog/gdt.h. */
#define SEL_NULL        0x00    /* Null selector.(널 셀렉터, 유효하지 않은 세그먼트) */
#define SEL_KCSEG       0x08    /* Kernel code selector.(커널 코드 세그먼트 셀렉터) */
#define SEL_KDSEG       0x10    /* Kernel data selector.(커널 데이터 세그먼트 셀렉터) */
#define SEL_UDSEG       0x1B    /* User data selector.(유저 데이터 세그먼트 셀렉터) */
#define SEL_UCSEG       0x23    /* User code selector.(유저 코드 세그먼트 셀렉터) */
#define SEL_TSS         0x28    /* Task-state segment.(TSS 셀렉터, 인터럽트나 유저/커널 전환 때 사용) */
#define SEL_CNT         8       /* Number of segments.(GDT 엔트리 개수) */

#endif /* threads/loader.h */
