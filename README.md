# Memory-pools

分别使用边界标识法和伙伴系统对内存池进行设计与实现。  
对操作系统和程序员而言，内存管理都是一个复杂而重要的问题。  
本项目就内存池即动态内存管理中涉及的一些基础技术和实现进行讨论。

## 一. 重要性

### 1. 性能优化

高效的内存分配与回收机制能够显著提升系统运行速度。在后面的内存池设计方案中，通过减少向操作系统频繁申请和释放内存的操作，可避免系统调用带来的额外开销，使得程序执行诸如数据处理、任务调度等关键流程时更加流畅。

### 2. 系统稳定性

内存碎片化控制至关重要。长时间运行的系统若缺乏有效内存管理，内存空间会变得支离破碎，导致后续即使总体空闲内存充足，但因无法找到连续大块内存来满足新的分配需求，引发内存分配失败，程序崩溃。

### 3. 资源利用率

精准的内存分配策略可避免过度分配。不同组件、模块按实际需求获取内存，而非盲目占用大量空间，使得有限的内存资源能够服务更多任务。

## 二. 可利用空间表及分配方法

根据系统运行的不同情况，可利用空间表可以有下列3种不同的结构形式：

- 系统运行期间所有用户请求分配的存储量大小相同。
- 系统运行期间用户请求分配的存储量有若干种大小的规格。
- 系统在运行期间分配给用户的内存块的大小不固定。

## 三. 内存池设计之边界标识法

边界标识法(boundary tag method)是操作系统中用以进行动态分区分配的一种存储管理方法，所有的空闲块链接在一个双向循环链表结构的可利用空间表中。

### 1. 表结构

![边界标识法表结构](https://github.com/user-attachments/assets/c858812d-e413-45c9-8c92-2c2271340268)

```c
typedef struct WORD {
    union {
        WORD *llink; //头部域,指向前驱结点
        WORD *uplink; //底部域,指向本结点头部
    };
    int tag; //标识,0空闲,1占用,头部和尾部都有
    int size; //头部域,块大小,以WORD为单位
    WORD *rlink; //头部域,指向后继结点
} WORD, head, foot, *Space; //Space:可利用空间指针类型
```

 ![记录所有空闲块的链表](https://github.com/user-attachments/assets/19933c00-0479-41f4-8ee7-805e5f23eaa6)

### 2.分配算法
假定我们采取首次拟合法,即从头指针位置查找第一个符合条件的结点.为了使整个系统更有效地运行,在边界标识法中还作了如下两个设计:
(1).分配的空闲块的容量为n个字WORD(包括头部和底部),若空闲块剩余的大小小于系统设定的边界值就将整个空闲块整块分配给用户;反之,则需要进行分割。同时,为了避免过多修改指针,约定将该结点中的高地址部分分配给用户。

(2).如果每次分配都从同一个结点开始查找的话,势必造成存储量小的结点密集在头指针pav所指结点附近,这同样会增加査询较大空闲块的时间。反之,如果每次分配从不同的结点开始进行查找,使分配后剩余的小块均匀地分布在链表中,则可避免上述弊病。实现的方法是,在每次分配之后,令指针pav指向刚进行过分配的结点的后继结点。

初始化过程如下:
```c
#define _CRT_SECURE_NO_WARNINGS //这一句必须放在第一行
#include <stdio.h> //标准输入输出文件
#include <stdlib.h>

//不带头节点的首次拟合法
typedef struct WORD {//字
    union {
        WORD* llink; //头部域,指向前驱结点
        WORD* uplink;//底部域,指向本结点头部
    };
    int tag;//标识,0空闲,1占用,头部和尾部都有
    int size;//头部域,块大小,以WORD为单位
    WORD* rlink;//头部域,指向后继结点
}WORD, * Space;//Space:可利用空间指针类型

#define SIZE 10000  //内存池的大小(WORD)
#define e 10 //碎片临界值,当剩余的空闲块小于e时,整个空闲块都分配出去

static Space FootLoc(Space p)//通过p返回p的尾
{
    return p + p->size - 1;
}

Space InitMem()//初始化内存池
{
    Space pav = (Space)malloc(SIZE * sizeof(WORD));//内存池
    //处理pav的头
    pav->llink = pav;
    pav->rlink = pav;
    pav->tag = 0;
    pav->size = SIZE;

    //处理p的尾
    Space p = FootLoc(pav);//p指向尾
    p->uplink = pav;
    p->tag = 0;

    return pav;
}
```
分配算法如下
```c
//向内存池pav,申请n个WORD,成功返回申请内存的地址,失败返回NULL
//利用首次拟合法
WORD* MyMalloc(Space* pav, int n)
{
    if (*pav == NULL)//内存池已经空了
        return NULL;
    Space p = *pav;
    do //首次拟合法,找第一个符合条件的空闲块
    {
        if (p->size >= n)//找到了
            break;
    } while (p != *pav);
    if (p->size < n)//没有满足条件的空闲块
        return NULL;
    if (p->size - n < e)//整个空闲块都分配
    {
        WORD* q = p;
        //q->llink,q->rlink,占用块不需要处理
        q->tag = 1;//占用
        //q->size 不改变
        p = FootLoc(p);//尾部
        //p->uplink不改变
        p->tag = 1;//占用
        //把q从链表中剔除
        if (q->rlink == q)//空闲链表中只有q这一个结点
            *pav = NULL;
        else//有多个结点
        {
            *pav = q->rlink;//头指针指向下一个结点
            q->llink->rlink = q->rlink;
            q->rlink->llink = q->llink;
        }
        return q;
    }
    else //空闲块一部分分配出去,把下面(高地址)分出去
    {
        *pav = p->rlink;//头指针指向下一个结点
        p->size -= n; //p分割后的新大小
        WORD *p2 = FootLoc(p);//新结点的尾
        p2->uplink = p;
        p2->tag = 0;
        //处理占用块
        WORD* q = p2+1;//指向需要返回的地址
        q->tag = 1;//占用
        q->size = n;//申请的大小
        p2 = FootLoc(q);//q的尾巴
        p2->uplink = q;
        p2->tag = 1;
        return q;
    }
}
```

####3.回收算法

用户释放占用块,为了使物理地址毗邻的空闲块结合成一个尽可能大的结点,则首先需要检查刚释放的占用块的左右紧邻是否为空闲块。本系统在每个内存区(无论是占用块或空闲块)的边界上都设有标志值,很容易检测这一点。

假设用户释放的内存区的头部地址为p,则与其低地址紧邻的内存区的底部地址为p-1;与其高地址紧邻的内存区的头部地址为p+p->size,它们中的标志域就表明了这两个邻区的使用状况:若(p-1)->tag=0;则表明其左邻为空闲块,若(p+p->size)->tag=0;则表明其右邻为空闲块。

若释放块的左、右邻区均为占用块,处理最为简单,只要将此新的空闲块作为一个结点插人到可利用空闲表中即可;

若只有左邻区是空闲块,则应与左邻区合并成一个结点;

若只有右邻区是空闲块,则应与右邻区合并成一个结点;

若左、右邻区都是空闲块,则应将3块合起来成为一个结点留在可利用空间表中。

![image](https://github.com/user-attachments/assets/87838ab8-28cf-4111-b627-3ed8e55adb8e)
p1,p2,p3,p4分别代表以上四种情况

![image](https://github.com/user-attachments/assets/28b2c48e-d173-4867-bd5d-bf41b74c71cc)

![image](https://github.com/user-attachments/assets/66a0cc3e-7506-4544-adcf-1439ab60059f)
每次从空闲节点的高地址分割

测试时请注意,实际的实现分配是从后往前

红色部分为"墙",防止越界

```c
void MyFree(Space* pav, WORD* p)//释放p
            p->llink = p2;
            p2->rlink = p;
        }
    }
    else if (pl->tag == 0 && pr->tag == 1)//左块为空闲块,右块为占用块
    {//只需要把p加到左块的下面即可
        WORD* q = pl->uplink;//左块的头
        q->size += p->size; //合并后空闲块的大小

        //处理新块的尾
        FootLoc(q)->tag = 0;
        FootLoc(q)->uplink = q;
    }
    else if (pl->tag==1 && pr->tag==0)//左块为占用块,右块为空闲块
    {//把右块从可利用空间表剔除,再把右块合并到p的下面,再把p插入到可利用空间表中
        //1.p的右块pr从可利用空间表剔除
        if (pr->rlink == pr)//只有一个空闲结点
        {
            *pav = NULL;
        }
        pr->llink->rlink = pr->rlink;
        pr->rlink->llink = pr->llink;

        //2.把右块合并到p的下面
            //处理p头部信息
        p->tag = 0;  
        p->size += pr->size;
            //处理p的尾巴信息
        FootLoc(p)->tag = 0;
        FootLoc(p)->uplink = p;

        //3.把p插入到可利用空间表(插入在pav的前面)
        if (*pav == NULL)//p是第一个结点
        {
            p->llink = p->rlink = p;
            *pav = p;
        }
        else//可利用空间表还有其它结点,将p插入在pav的前面,即p成为可利用空间表的最后一个结点
        {
            WORD* p1 = *pav;//第一个结点
            WORD* p2 = p1->llink;//最后一个结点

            //把p插入在p1和p2的中间,即p1的前面,p2的后面
            p->rlink = p1;
            p1->llink = p;
            p->llink = p2;
            p2->rlink = p;
        }
    }
    else //左块为空闲块,右块也为空闲块
    {//把右块从链表中剔除,再把左,p,右三块合并
        //1.把右块从链表中剔除
        pr->llink->rlink = pr->rlink;
        pr->rlink->llink = pr->llink;

        //2.把左,p,右合并
           //处理新块的头
        pl = pl->uplink;//找到pl(左块)的头
        pl->size = pl->size + p->size + pr->size;
           //处理新块的尾巴
        FootLoc(pl)->uplink = pl;
        FootLoc(pl)->tag = 0;
    }
}
```
总之,边界标识法由于在每个结点的头部和底部设立了标识域,使得在回收用户释放的内存块时,很容易判别与它毗邻的内存区是否是空闲块,且不需要查询整个可利用空间表便能找到毗邻的空闲块与其合并;再者,由于可利用空间表上结点既不需依结点大小有序,也不需依结点地址有序,则释放块插人时也不需查找链表。由此,不管是哪一种情况,回收空闲块的时间都是个常量,和可利用空间表的大小无关。惟一的缺点是增加了结点底部所占的存储量。
 
## 四.内存池设计之伙伴系统

伙伴系统(buddysystem)是操作系统中用到的另一种动态存储管理方法。它和边界标识法类似,在用户提出申请时,分配一块大小“恰当”的内存区给用户;反之,在用户释放内存区时即回收。所不同的是:在伙伴系统中,无论是占用块或空闲块,其大小均为2的k次幂(k为某个正整数)。例如:当用户申请n个字的内存区时,分配的空闲块大小为2^k个字(2^(k-1)<n<=2^k)。若总的可利用内存容量为 2^m个字,则空闲块的大小只可能为2^0、2^1…、2^m。

### 1.表结构
假设系统的可利用内存空间容量为2^m个字,则在开始运行时整个内存区是一个大小为2^m的空闲块,在运行了一段时间之后,被分隔成若干占用块和空闲块。为了再分配时查找方便起见,我们将所有大小相同的空闲块放在一张子表中,每个子表是一个双向链表,这样的链表可能有m+1个,将这m十1个表头指针用顺序表组织成一个表,这就是伙伴系统中的可利用空间表。
双向链表中的结点结构如下:

```c
#define m 16 //内存池总容量2^16即65536字WORD_b

typedef struct WORD_b{   //伙伴系统的字 (结点结构)
    struct WORD_b *llink;//前驱指针
    int tag;             //标识,0空闲,1占用
    int kval;            //块大小,2的幂次k(保存的是k值)
    struct WORD_b *rlink;//后继指针
}WORD_b;
typedef struct HeadNode{//可利用空间表
    int nodesize;       //该链表的空闲块大小
    WORD_b *first;      //链表表头指针
}FreeList[m+1];     
```

![image](https://github.com/user-attachments/assets/b18ac496-6e45-443a-8a71-02a6c23761cc)
a空闲块  b表的初始状态 c分配前的表 d分配后的表

其中head为结点头部,是一个由4个域组成的记录,其中的 llink 域和rlink域分别指向同一链表中的前驱和后继结点;tag 域为值取"0"空闲,"1"占用,标志域,kval域的值为2的幂次k;space是一个大小为2^k-1个字的连续内存空间(和前面类似,仍假设 head 占一个字的空间)。

### 2.初始化

```c
#define _CRT_SECURE_NO_WARNINGS //这一句必须放在第一行
#include <stdio.h> //标准输入输出文件
#include <stdlib.h>
#include <assert.h>

#define m 16 //内存池总容量2^16即65536字WORD_b

typedef struct WORD_b {   //伙伴系统的字 (结点结构)
    struct WORD_b* llink;//前驱指针
    int tag;             //标识,0空闲,1占用
    int kval;            //块大小,2的幂次k(保存的是k值)
    struct WORD_b* rlink;//后继指针
}WORD_b,*Space_b;
typedef struct HeadNode {//可利用空间表
    int nodesize;       //该链表的空闲块大小
    WORD_b* first;      //链表表头指针
}FreeList[m + 1];

static Space_b pav;//内存池的起始地址

void InitMem(FreeList *pf)//创建并初始化内存池
{
    //创建内存池
    g_pav = (WORD_b*)malloc((1<<m) * sizeof(WORD_b));//1<<1->2 ,1<<2->4,1<<3->8

    assert(g_pav != NULL);
    if (g_pav == NULL)
    {
        printf("内存池初始化失败!!!\n");
        return;
    }
    //初始化内存池
    g_pav->llink = g_pav->rlink = g_pav;
    g_pav->tag = 0;
    g_pav->kval = m;


    //初始化可利用空间表
    for (int i = 0; i < m+1; i++)
    {
        (*pf)[i].nodesize = 1 << i;
        (*pf)[i].first = NULL;
    }
    (*pf)[m].first = g_pav;//把内存接到可利用空间表
}

int main()
{
    FreeList fl;
    InitMem(&fl);

	return 0;
}
```

![image](https://github.com/user-attachments/assets/3907f9da-9aef-450a-addb-e8174318884b)





