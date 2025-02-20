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

static Space_b g_pav;//内存池的起始地址

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

//分配内存
//pf可利用空间表,n申请的字(WORD_b)
WORD_b* MyMalloc(FreeList* pf, int n)
{
    if (n <= 0)
        return NULL;
    int i;
    for (i = 0; i < m + 1; i++)//找合适的空闲块
    {
        //空闲块大小>=n且后面有结点
        if ((*pf)[i].nodesize >= n && (*pf)[i].first != NULL)
            break;
    }
    if (i == m + 1)//没有找到
        return NULL;
    //处理i后面的第一个结点(1.从链表中剔除,2.分裂剩余部分)
    WORD_b* p = (*pf)[i].first;
    if (p->rlink == p)//唯一的结点
        (*pf)[i].first = NULL;
    else
    {
        (*pf)[i].first = p->rlink;//头指针指向下一个结点
        //把p从链表中剔除
        p->llink->rlink = p->rlink;
        p->rlink->llink = p->llink;
    }

    //分裂剩余部分(从下往上,从大到小)
    for (i=i-1; (*pf)[i].nodesize >= n; i--)
    {
        WORD_b* q = p + (*pf)[i].nodesize; //分裂的新节点
        q->llink = q->rlink = q;
        q->tag = 0;
        q->kval = i;

        (*pf)[i].first = q;
    }

    //处理分配出去的内存
    p->tag = 1;
    p->kval = i + 1;

    return p;
}

//释放占用块p到可利用空间表pf中
void MyFree(FreeList* pf, WORD_b* p)
{
    if (p == NULL)
        return;
    WORD_b* q; //伙伴地址
    int flg = 0;//flg=0,p是左块;flg=1,p是右块
    int i;
    for (i = p->kval; i < m; i++)
    {
        //判断自己是谁(是左块还是右块)?
        if ((p - g_pav) % (1 << (p->kval + 1)) == 0)//p-g_pav:相对地址,  p是左块
        {
            flg = 0;//p是左块
            q = p + (1 << p->kval);
        }
        else //p是右块
        {
            flg = 1;//p是右块
            q = p - (1 << p->kval);
        }

        if (q->tag == 1)//伙伴是占用块
            break;
        else //伙伴是空闲块,需要合并
        {
            //把伙伴q从相应的链表中删除
            if (q->rlink == q)//当前链表只有一个结点
                (*pf)[i].first = NULL;
            else //不止一个结点,需要把q从链表中剔除
            {
                if ((*pf)[i].first == q)//q是链表最前面的结点
                    (*pf)[i].first = q->rlink;
                //把q剔除
                q->llink->rlink = q->rlink;
                q->rlink->llink = q->llink;
            }
            if (flg == 0)//p是左块,把伙伴合并到p中
            {
                p->tag = 0;
                p->kval += 1; //指数+1
            }
            else //p是右块,把p合并到伙伴q中
            {
                q->kval += 1;
                p = q;
            }
        }
    }
    
    //把p插入到相应的位置
    if ((*pf)[i].first == NULL)//p是插入的第一个结点
    {
        p->llink = p->rlink = p;
        (*pf)[i].first = p;
    }
    else //p不是当前链表的第一个结点
    {
        WORD_b* p1 = (*pf)[i].first->llink;//最后一个结点,也将成为p的前驱
        WORD_b* p2 = (*pf)[i].first;//第一个结点,也将称为p的后继
        p->llink = p1;
        p->rlink = p2;
        p1->rlink = p;
        p2->llink = p;
    }
}

//输出占用块信息
void Show(WORD_b *p)
{
    if (p == NULL)
        return;
    printf("占用块,标识=%d,大小=%d\n",p->tag,1<<p->kval);
}

//输出可利用空间表的所有结点信息
void ShowPav(FreeList* pf)
{
    WORD_b* p;
    for (int i = 0; i < m + 1; i++)
    {
        if ((*pf)[i].first == NULL)
            continue;
        p = (*pf)[i].first;
        printf("空闲块大小:%d\n",1<<p->kval);
        do
        {
            printf("    空闲块:标记=%d,大小=%d\n",p->tag,1<<p->kval);
            p = p->rlink;
        } while (p != (*pf)[i].first);
    }
}


int main()
{
    /*for (int i = 0; i < 10; i++)
    {
        printf("2^%d = %d\n",i,1<<i);
    }*/

    FreeList fl;
    InitMem(&fl);

    ShowPav(&fl);

    printf("-----------------------\n");

    WORD_b* p1 = MyMalloc(&fl,500);
    WORD_b* p2 = MyMalloc(&fl, 500);
    WORD_b* p3 = MyMalloc(&fl, 500);
    WORD_b* p4 = MyMalloc(&fl, 500);
    Show(p1);
    Show(p2);
    Show(p3);
    Show(p4);
    ShowPav(&fl);

    printf("-----------------------\n");
    MyFree(&fl,p2); p2 = NULL; //p2和p3不是伙伴
    MyFree(&fl, p3); p3 = NULL;
    ShowPav(&fl);

	return 0;
}