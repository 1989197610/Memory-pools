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
    //Space pav = (Space)malloc(SIZE * sizeof(WORD));//内存池
    //在内存池的前后多建"一堵墙",防止回收合并时越界
    Space pav = (Space)malloc((SIZE+2) * sizeof(WORD));//内存池+2堵墙
    pav->tag = 1;//左墙
    pav++;

    //处理pav的头
    pav->llink = pav;
    pav->rlink = pav;
    pav->tag = 0;
    pav->size = SIZE;

    //处理p的尾
    Space p = FootLoc(pav);//p指向尾
    p->uplink = pav;
    p->tag = 0;

    (p + 1)->tag = 1;//右墙

    return pav;
}

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
        p = p->rlink;
    } while (p != *pav);

    if (p->size < n)//没有满足条件的空闲块
        return NULL;

    if (p->size - n < e)//整个空闲块都分配
    {
        WORD* q = p;
        //q->llink,q->rlink,占用块不需要处理
        q->tag = 1;//占用
        //q->size 不改变
        p = FootLoc(p);
        //p->uplink不改变
        p->tag = 1;//占用

        //把q从链表中剔除
        if (q->rlink == q)//空闲链表中只有q这一个结点
            *pav = NULL;
        else//有多个结点
        {
            *pav = q->rlink;

            q->llink->rlink = q->rlink;
            q->rlink->llink = q->llink;
        }

        return q;
    }
    else //空闲块一部分分配出去,把下面(高地址)分出去
    {
        *pav = p->rlink;
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

void MyFree(Space* pav, WORD* p)//释放p
{
    WORD* pl = p - 1;//左块的尾巴
    WORD* pr = FootLoc(p) + 1;//右块的头
    if (pl->tag == 1 && pr->tag == 1)//左块占用,右块占用,直接插入
    {
        p->tag = 0; //释放后是空闲块
        FootLoc(p)->tag = 0;
        if (*pav == NULL)//可利用空间表为NULL,p是第一个结点
        {
            *pav = p;
            p->llink = p->rlink = p;
        }
        else
        { //将p插入在pav的前面,即p成为可利用空间表的最后一个结点
            WORD* p1 = *pav;//第一个结点
            WORD* p2 = p1->llink;//最后一个结点

            //把p插入在p1和p2的中间,即p1的前面,p2的后面
            p->rlink = p1;
            p1->llink = p;
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

void Show(WORD* p)//测试函数,输出p的数据
{
    printf("p的地址:%p,p->tag:%d,p->size:%d(WORD),p的尾巴的tag:%d\n",
        p,p->tag,p->size,FootLoc(p)->tag);
}

void ShowMem(Space pav)//输出内存池的所有空闲块
{
    if (pav == NULL)
    {
        printf("内存池已空\n");
        return;
    }
    Space p = pav;
    do
    {
        printf("空闲块,地址:%p,tag:%d,size:%d,尾巴tag:%d\n",
            p,p->tag,p->size,FootLoc(p)->tag);
        p = p->rlink;
    } while (p != pav);
}

int main()
{
    Space pav = InitMem();//创建并初始化好的内存池
    WORD* p1 = MyMalloc(&pav, 1000);
    WORD* p2 = MyMalloc(&pav, 1500);
    WORD* p3 = MyMalloc(&pav, 500);
    WORD* p4 = MyMalloc(&pav, 1000);
    WORD* p5 = MyMalloc(&pav, 1000);

    Show(p1);
    Show(p2);
    Show(p3);
    Show(p4);
    Show(p5);
    ShowMem(pav);

    printf("-----------\n");
    /*MyFree(&pav, p2); p2 = NULL;
    MyFree(&pav, p1); p1 = NULL;
    MyFree(&pav, p3); p3 = NULL;*/
    //测试左右都是空闲块
    MyFree(&pav, p2); p2 = NULL;
    MyFree(&pav, p4); p4 = NULL;
    MyFree(&pav, p3); p3 = NULL;

    MyFree(&pav, p1); p1 = NULL;
    MyFree(&pav, p5); p5 = NULL;
    ShowMem(pav);

	return 0;
}