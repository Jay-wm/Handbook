#include "configbase.h"
#include "Gb103Jni.h"
#include "zsocket.h"
#include <stdio.h>
#include <string>
enum
{
    COMM_OK,
    COMM_ERR_REMOTEHOSTCLOSED,
    COMM_ERR_PKT_ERR,
};

struct dllprivate
{
	dllprivate()
	{
		pthread_mutex_init(&mtx, NULL);
	}

	~dllprivate()
	{
		pthread_mutex_destroy(&mtx);
    }

	void init()
	{
		txcount		= 0;
		rxcount		= 0;
		lastRxCount = 0;
		rii			= 0;
	}

	DeviceItem* getDevice( BYTE devaddr)
	{
		for ( GroupList::iterator it = groupList.begin();
			it != groupList.end(); it++)
		{
			if ( (*it)->GroupType() != GroupBase::GROUP_TYPE_DEVICE_CONFIG)
				continue;

			for (ItemList::iterator devit = (*it)->m_ItemList.begin();
				devit != (*it)->m_ItemList.end(); devit++)
			{
				DeviceItem* pDevice = (DeviceItem*)devit->second;
				if ( pDevice->devaddr == devaddr)
				{
					return pDevice;
				}
			}
		}
		return NULL;
	}

	GroupList* getGroupList(BYTE addr, BYTE cpu)
	{
		// 1. ��վ��
		if(addr == 0)
			return &groupList;

		// 2. װ����
		GroupList::iterator it = groupList.begin();
		for (; it != groupList.end(); it++)
		{
			if ( (*it)->GroupType() == GroupBase::GROUP_TYPE_DEVICE_CONFIG)
				break;
		}

		if ( it == groupList.end())
		{
			printf("get group addr %d failed", addr);
			return NULL;
		}

		DeviceItem* pDevice = NULL;
		for (ItemList::iterator devit = (*it)->m_ItemList.begin(); 
			devit != (*it)->m_ItemList.end(); devit++)
		{
			pDevice = (DeviceItem*)devit->second;
			if ( pDevice->devaddr == addr)
			{
				for (vector<CpuItem>::iterator cpuIt = pDevice->cpus.begin();
					cpuIt != pDevice->cpus.end(); cpuIt++)
				{
					if ( cpuIt->cpuno == cpu)
					{
						return &(cpuIt->m_groups);
					}
				}
			}
		}

		return NULL;
	}

	GroupBase* getGroup(BYTE addr, BYTE cpu, BYTE gno)
	{
		GroupList* pList = getGroupList(addr, cpu);
		if ( pList == NULL)
			return NULL;

		for (GroupList::iterator it = pList->begin();
			it != pList->end(); it++)
		{
			if ( (*it)->groupNo == gno)
			{
				return (*it);
			}
		}

		return NULL;
	}

	GroupBase* findGroup( BYTE addr, BYTE cpu, int groupType )
	{
		GroupList* pList = getGroupList(addr, cpu);
		if ( pList == NULL)
			return NULL;

		for (GroupList::iterator it = pList->begin();
			it != pList->end(); it++)
		{
			if ( (*it)->GroupType() == groupType)
			{
				return (*it);
			}
		}

		return NULL;
	}

	void clearConfig()
	{
		for (GroupList::iterator it =groupList.begin(); it != groupList.end(); it++)
		{
			GroupBase* g = *it;
			if(g->GroupType() == GroupBase::GROUP_TYPE_DEVICE_CONFIG)// �����豸̨��
			{
				ItemList* ItemListmap = &((*it)->m_ItemList);
				for (map<int, ItemBase*>::iterator ite =ItemListmap->begin(); ite != ItemListmap->end(); ite++ )
				{
					DeviceItem* stbase = (DeviceItem*)ite->second;
					for(vector<CpuItem>::iterator itcpu =stbase->cpus.begin(); itcpu !=stbase->cpus.end(); itcpu++)//cpu
					{
						for(GroupList::iterator itgroup = itcpu->m_groups.begin(); itgroup !=itcpu->m_groups.end(); itgroup++)//��
						{
							ItemList* Itemmap = &((*itgroup)->m_ItemList);
							for(map<int, ItemBase*>::iterator itemmap =Itemmap->begin(); itemmap != Itemmap->end(); itemmap++)
								delete itemmap->second;
							delete *itgroup;
						}
						itcpu->m_groups.clear();
					}

					delete stbase;
				}
				
			}
			else // һ���豸̨��
			{
				ItemList* ItemListmap = &((*it)->m_ItemList);
				for (map<int, ItemBase*>::iterator ite =ItemListmap->begin(); ite != ItemListmap->end(); ite++ )
					delete ite->second;
			}

			delete g;
		}

		groupList.clear();
	}

	ZTcpSocket  s;
	ZJ104_FRM	tx;
	ZJ104_FRM	rx;
    map<dllprivate*, IedInfo*> m_data;

	BYTE		rii;
	int			txcount;
	int			rxcount;
	int			lastRxCount;
	pthread_mutex_t mtx;
	GroupList groupList;
	ConfigFactory factory;		//���������Ŀ����
};


typedef enum _LINK_104_FRAME_TYPE
{
	LINK_104_I_FRAME		= 0x0000,	/* I֡������ASDU ����Ϣ֡ */
	LINK_104_S_FRAME		= 0x0001,	/* S֡��104 Э��ȷ��֡    */
	LINK_104_U_FRAME		= 0x0003,	/* U֡��104 Э�鹦��֡������ ֹͣ ���� */
	LINK_104_FRAME_FORMAT	= 0x0003,	/* ��ʽ��֡��ͨ���뱾֡����ó�֡����  */
	LINK_104_U_START		= 0x0007,	/* �������� */
	LINK_104_U_START_CMF	= 0x000B,	/* ����ȷ�� */
	LINK_104_U_STOP			= 0x0013,	/* ֹͣ���� */
	LINK_104_U_STOP_CMF		= 0x0023,	/* ֹͣȷ�� */	
	LINK_104_U_TEST			= 0x0043,	/* �������� */
	LINK_104_U_TEST_CMF		= 0x0083,	/* ����ȷ�� */
}LINK_104_FRAME_TYPE;


static inline bool zj104_frm_type( int i_nRemoteTxCnt )
{
	return LINK_104_U_START_CMF==(i_nRemoteTxCnt&LINK_104_U_START_CMF);
}

static inline void fill_test_ack(ZJ104_FRM* f, int txcount)
{
	f->apci.stx	= ZJ104_FRM::STX;
	f->apci.apdulen	= 4;
	f->apci.txcount	= LINK_104_U_TEST_CMF;
	f->apci.rxcount	= txcount << 1;
}

static inline void fill_ack(ZJ104_FRM* f, int txcount)
{
	f->apci.stx	= ZJ104_FRM::STX;
	f->apci.apdulen	= 4;
	f->apci.txcount	= 0x01;
	f->apci.rxcount	= txcount << 1;
}

static inline void fill_get_title(ZJ104_FRM* f, BYTE addr, BYTE cpu, int txcount, int rxcount, BYTE rii)
{
	f->apci.stx = ZJ104_FRM::STX;
	f->apci.apdulen = 4 + 8;
	f->apci.txcount = txcount << 1;
	f->apci.rxcount	= rxcount << 1;
	f->asdu.addr = addr;
	f->asdu.cpu = cpu;
	f->asdu.typ	= 0x15;
	f->asdu.vsq	= 0x81;
	f->asdu.cot	= 0x2A;
	f->asdu.fun	= 0xfe;
	f->asdu.inf	= 0xf0;
	f->asdu.rii	= rii;
}
static inline void fill_get_group(ZJ104_FRM* f, BYTE addr, BYTE cpu, BYTE gno, BYTE kod, int txcount, int rxcount, BYTE rii)
{
	f->apci.stx = ZJ104_FRM::STX;
	f->apci.apdulen = 4 + 8 + sizeof(ASDU10_PACK);
	f->apci.txcount = txcount << 1;
	f->apci.rxcount	= rxcount << 1;
	f->asdu.addr = addr;
	f->asdu.cpu = cpu;
	f->asdu.typ	= 0x15;
	f->asdu.vsq	= 0x81;
	f->asdu.cot	= 0x2A;
	f->asdu.fun	= 0xfe;
	f->asdu.inf	= 0xf1;
	f->asdu.rii	= rii;

	ASDU10_PACK* asdu10 = (ASDU10_PACK*)f->asdu.data;
	asdu10->ngd = 1;
	asdu10->grp[0].gno = gno;
	asdu10->grp[0].ino = 0;
	asdu10->grp[0].kod = kod;
}



static inline int zj104_send(dllprivate* dp)
{
	int len = frm_len(&dp->tx);
	int ret = dp->s.write((char*)&dp->tx, len);
	if(ret <= 0)
	{
		dp->s.close();
		return COMM_ERR_REMOTEHOSTCLOSED;
	}

	dp->lastRxCount = dp->rxcount;
	return COMM_OK;
}
/*
* mod by cx on20121103 
* 104 ��Լ�Ļ�������ͨ��Э��
* 1���յ�S ֡���� U  ֡������������ֱ�ӷ���
* 2���յ�I ֡��ֻ���������б���ļ�������֡��������Ϣ��������
* 3������104�������յ�8��I��ʽ��APDU��������ȷ��
û�յ�8 ������֡�󣬱�����Ӧһ��S ֡��
*/
static inline int zj104_recv(dllprivate* dp)
{
	int ret = 0;
	ZJ104_FRM& rx = dp->rx;
	do
	{
		ret = dp->s.read((char*)&rx, 7);
		if(ret != 7)
		{
			return COMM_ERR_REMOTEHOSTCLOSED;
		}
		if(rx.apci.stx != ZJ104_FRM::STX)
		{
			dp->s.clearRcvBuf();
			unsigned char* tmp = (unsigned char*)&rx;
			printf("unexpected:%02X %02X %02X %02X %02X %02X %02X\n", 
				tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5], tmp[6]);
			return COMM_ERR_PKT_ERR;
		}

		/* ��֡��S֡����U֡�������� */
		if ( rx.apci.apdulen == 4 )
		{
			if ( zj104_frm_type(rx.apci.txcount) )// ����ȷ��
			{
				/* U֡��״̬֡ */
				return COMM_OK;
			}
			else if(LINK_104_U_TEST == (rx.apci.txcount&LINK_104_U_TEST))/*��������*/
			{
				fill_test_ack(&dp->tx, rx.apci.rxcount);
				zj104_send(dp);
				return COMM_OK;
			}
			else if(LINK_104_U_TEST_CMF == (rx.apci.txcount&LINK_104_U_TEST_CMF))/*����ȷ��*/
			{
				return COMM_OK;
			}
			else if(LINK_104_U_STOP_CMF == (rx.apci.txcount&LINK_104_U_STOP_CMF)) /*ֹͣȷ��*/
			{
				return COMM_OK;
			}
			else if(LINK_104_U_STOP == (rx.apci.txcount&LINK_104_U_STOP)) /*ֹͣ����*/
			{
				return COMM_OK;
			}
			continue;
		}

		/* �յ�һ��֡(I֡) */
		ret	= dp->s.read((char*)rx.buf, rx.apci.apdulen - 4);
		if(ret != rx.apci.apdulen - 4)
		{
			//printf("recv body [%d:%d] failed\n", rx.apci.apdulen - 4, ret);
			return COMM_ERR_REMOTEHOSTCLOSED;
		}

		int peerSendNo = (rx.apci.txcount >> 1);
		++peerSendNo;
		dp->rxcount = peerSendNo;

		//δ������������⣺if ( dp->rxcount - dp->lastRxCount >= MAX_UNCONFIRMED_FRM_NUMS)
		{
			fill_ack(&dp->tx, dp->rxcount);
			ret = zj104_send(dp);
		}

		break;
	}while(1);

	return COMM_OK;
}

static inline void	fill_reset(ZJ104_FRM* f)
{
	f->apci.stx	= ZJ104_FRM::STX;
	f->apci.apdulen	= 4;
	f->apci.txcount = 0x07;
	f->apci.rxcount	= 0;
}


///< ���������
static int requestTitle(dllprivate* lpUnitCfg, BYTE addr, BYTE cpu)
{
	fill_get_title(&(lpUnitCfg->tx), addr, cpu, lpUnitCfg->txcount++, lpUnitCfg->rxcount, lpUnitCfg->rii++);
	int ret = zj104_send(lpUnitCfg);
	while(ret == COMM_OK)
	{
		ret = zj104_recv(lpUnitCfg);
		if(ret != COMM_OK)
			break;

		if(lpUnitCfg->rx.apci.apdulen == 4)
			continue;

		if(lpUnitCfg->rx.asdu.typ != 0x0A)
		{
			//onEvent(dp);
			continue;
		}

		if(lpUnitCfg->rx.asdu.cot == 0x2B)
		{
			break;
		}

		ASDU10_PACK* pHeader = (ASDU10_PACK*)lpUnitCfg->rx.asdu.data;
		GroupList* pGrpList = lpUnitCfg->getGroupList(lpUnitCfg->rx.asdu.addr, lpUnitCfg->rx.asdu.cpu);

		if ( pGrpList == NULL)
		{
			printf("��ȡ�����Ϊ��:uid = %d, cpu = %d", lpUnitCfg->rx.asdu.addr, lpUnitCfg->rx.asdu.cpu);
		}
		else
		{
			//printf("��ʼ������ȡ�����:uid = %d, cpu = %d", lpUnitCfg->rx.asdu.addr, lpUnitCfg->rx.asdu.cpu);
			char group[256];
			STD103_GROUP*  pTemp = pHeader->grp;
			for (int i = 0; i < pHeader->count(); ++i, pTemp = (STD103_GROUP*)((char *)pTemp + pTemp->sLen()))
			{

				strncpy(group, (char*)pTemp->gid, pTemp->dLen());
				group[pTemp->dLen()] = 0;

				GroupBase * pGrp = lpUnitCfg->factory.createGroup(pTemp, group);
				if (pGrp != NULL)
				{
					pGrpList->push_back(pGrp);
					pGrp->SetValue(pTemp->gno, (char*)pTemp->gid, pTemp->len);
					/*printf("��ȡ�����:group[addr, cpu, group=%d, %d, %d]:%s\n",
						lpUnitCfg->rx.asdu.addr,
						lpUnitCfg->rx.asdu.cpu,
						pGrp->groupNo, pGrp->groupDesc);*/
				}
			}
		}

		if(!pHeader->cont())
			break;
	}
	//printf("��ȡ����ⷵ��:ret = %d", ret);
	return ret;
}


static int requestGroup(dllprivate * lpUnitCfg, BYTE addr, BYTE cpu, BYTE gno, BYTE kod)
{
	fill_get_group(&lpUnitCfg->tx, addr, cpu, gno, kod, lpUnitCfg->txcount++, lpUnitCfg->rxcount, lpUnitCfg->rii++);
	int ret = zj104_send(lpUnitCfg);
	while(ret == COMM_OK)
	{
		ret = zj104_recv(lpUnitCfg);
		if(ret != COMM_OK)
			break;

		if(lpUnitCfg->rx.apci.apdulen == 4)
			continue;

		if(lpUnitCfg->rx.asdu.typ != 0x0A)
		{
			//onEvent(dp);
			continue;
		}

		if(lpUnitCfg->rx.asdu.cot == 0x2B)
		{
			break;
		}
		//printf("��ȡ������Ϣ�����ͱ�ʶ��%d", lpUnitCfg->rx.asdu.typ);
		ASDU10_PACK* pHeader = (ASDU10_PACK*)lpUnitCfg->rx.asdu.data;
		GroupList* pGrpList = lpUnitCfg->getGroupList(lpUnitCfg->rx.asdu.addr, lpUnitCfg->rx.asdu.cpu);
		if ( pGrpList == NULL)
		{
			printf("δȡ����Ӧ���鼯�ϣ�devaddr=%d", lpUnitCfg->rx.asdu.addr);
		}
		else
		{
			STD103_GROUP*  pTemp = pHeader->grp;
			for (int i = 0; i < pHeader->count(); ++i, pTemp = (STD103_GROUP*)((char *)pTemp + pTemp->sLen()))
			{
				GroupBase* pGroup = lpUnitCfg->getGroup(lpUnitCfg->rx.asdu.addr, lpUnitCfg->rx.asdu.cpu, pTemp->gno);
				if (pGroup == NULL)
				{
					printf("��ȡ������Ϊ�գ�devaddr=%d,groupNo=%d", lpUnitCfg->rx.asdu.addr, pTemp->gno);
					continue;
				}
				if (pTemp->ino == 0)
					continue;

				ItemList::iterator it = pGroup->m_ItemList.find(pTemp->ino);
				ItemBase * pItem = NULL;
				if (it == pGroup->m_ItemList.end())
				{
					pItem = lpUnitCfg->factory.createItem(pGroup->GroupType(), pTemp);
					if (pItem != NULL)
						pGroup->m_ItemList[pTemp->ino] = pItem;
				}
				else
				{
					pItem = it->second;
				}
				if (pItem == NULL)
					continue;
				pItem->SetValue(pTemp);
				/*printf("��ȡ����������:item[addr, cpu, gno, ino=%d, %d, %d, %d]:%s\n",
							lpUnitCfg->rx.asdu.addr, lpUnitCfg->rx.asdu.cpu,
							pTemp->gno, pItem->itemNo, (pItem->itemDesc));*/
			}
		}
		if(!pHeader->cont())
			break;
	}

    //printf("��ȡ�����ݷ���:ret = %d", ret);
	return ret;
}
static int requestGroupInfos(dllprivate * lpUnitCfg, BYTE addr, BYTE cpu, BYTE gno)
{
	int ret = requestGroup(lpUnitCfg, addr, cpu, gno, STD103_KOD_GETDESCR);
	if(ret == COMM_OK)
		ret = requestGroup(lpUnitCfg, addr, cpu, gno, STD103_KOD_GETPROP);
	return ret;
}

/*
**  �������ƣ�int requestDevice(dllprivate * lpUnitCfg, BYTE addr)
**  �������ܣ������豸��Ϣ -- ����Ϣ����Ŀ��Ϣ��δ��ȡ��ֵ
*/
static int requestDevice(dllprivate * lpUnitCfg, BYTE addr)
{
	int ret = COMM_OK;
	DeviceItem* pDevice = lpUnitCfg->getDevice(addr);
	if ( pDevice == NULL)
		return ret;

	//װ�������
	for (vector<CpuItem>::iterator it = pDevice->cpus.begin();
		it != pDevice->cpus.end(); it++)
	{
		ret = requestTitle(lpUnitCfg, addr, it->cpuno);
		if ( ret != COMM_OK)
			break;
	}

	//װ������Ŀ
	if(ret == COMM_OK)
	{
		for (vector<CpuItem>::iterator it = pDevice->cpus.begin();
			it != pDevice->cpus.end(); it++)
		{
			for (GroupList::iterator git = it->m_groups.begin();
				git != it->m_groups.end(); git++)
			{
				ret = requestGroupInfos(lpUnitCfg, addr, it->cpuno, (*git)->groupNo);
				if ( ret != COMM_OK)
					break;
			}
		}
	}
	
	return ret; 
}

dllprivate* StartMonitor(const char* host, unsigned short port)
{
	int ret = COMM_OK;
    dllprivate* dp = new dllprivate;

	if(1)
	{
		if(port == 0)
		{
			port = 19090;
		}
		if(!dp->s.open(host, port))
		{
			printf("connect %s:% ʧ��\n", host, port);
			return dp;
		}

		dp->s.setRxTmout(30000);// ��ʱ30��
		dp->init();

		fill_reset(&(dp->tx));
		ret = zj104_send(dp);
		if(ret == COMM_OK)
			ret = zj104_recv(dp);

		if(ret != COMM_OK)
		{
			dp->s.close();
			ret = COMM_ERR_REMOTEHOSTCLOSED;
		}

		printf("���� %s:%d ret = %d\n", host, port, ret);

		if(ret == COMM_OK)// ������� 
		{
			dp->m_data;// gi=0
			for (map<dllprivate*, IedInfo*>::iterator it = dp->m_data.begin(); it != dp->m_data.end(); it++)
			{
				it->second->gi = 0;
			}
		}
	}
	return dp;
}

 int GetConfig(const char* host_str, string& out)
{
    //const char* host_str = "172.17.2.75";
    //string out;
    char buffer[1024];
    unsigned short post = 19090;
    char errorMessage[1024];

    dllprivate* lpUnitCfg = new dllprivate;
    lpUnitCfg->clearConfig();
    lpUnitCfg = StartMonitor(host_str,post);

	int ret = requestTitle(lpUnitCfg, 0, 1);
	if(ret != COMM_OK)	
	{
		sprintf(errorMessage,"������վ�����ʧ��...");
		return ret;
	}

    GroupBase* pGroup = lpUnitCfg->findGroup(0, 1, GroupBase::GROUP_TYPE_DEVICE_CONFIG);
    if (pGroup == NULL)
    {
        sprintf(errorMessage,"��վ����Ϊ��...");
        return ret;
    }
	ret = requestGroupInfos(lpUnitCfg, 0, 1, pGroup->groupNo);
	if(ret != COMM_OK)
	{
		sprintf(errorMessage,"������վ�豸ʧ��...");
		return ret;
	}

	for ( GroupList::iterator it = lpUnitCfg->groupList.begin();
		it != lpUnitCfg->groupList.end(); it++)
	{
		int type = (*it)->GroupType();
		switch(type)
		{
		case GroupBase::GROUP_TYPE_DEVICE_CONFIG:// ���󱣻��嵥
			{				
                int i = 0;
				for (ItemList::iterator devit = (*it)->m_ItemList.begin();
					devit != (*it)->m_ItemList.end(); devit++, ++i)
				{
					DeviceItem* pDevice = (DeviceItem*)devit->second;
					ret = requestDevice(lpUnitCfg, pDevice->devaddr);
					if(ret == COMM_OK)
                    {
                        printf("����%s�������\n", (pDevice->itemDesc));
                    }
					else
					{
						sprintf(errorMessage,"����%s����ʧ��\n", (pDevice->itemDesc));
						return ret;
					}
				}
			}
			break;
		default: break;
		}
	}
    bool mark = false;//����Ƿ��ٵ�����
	if(ret == COMM_OK)
	{
        for( GroupList::iterator it = lpUnitCfg->groupList.begin();
		it != lpUnitCfg->groupList.end(); it++)
        {
            GroupBase* gb = *it;
            if(GroupBase::GROUP_TYPE_DEVICE_CONFIG==gb->GroupType())
            {
                mark = true;
                sprintf(buffer,"{'result':1, 'message':'','content':[");
                out = buffer;
                int i = 0;
		        for (ItemList::iterator devit = gb->m_ItemList.begin();
			        devit != gb->m_ItemList.end(); devit++, ++i)
		        {
                    DeviceItem* pDevice = (DeviceItem*)devit->second;
                    sprintf(buffer,"{'id':%d, 'name':'%s'},",(pDevice->devaddr),pDevice->itemDesc);
                    out += buffer;
                }
            }
            else continue;
        }        
	}
    if(true==mark)
    {
        out = out.substr(0,out.length()-1);
        sprintf(buffer,"]}");
        out += buffer;
        const char * p= out.data();
        //printf("%s",p);
    }
    else
    {
        sprintf(buffer,"{'result':0, 'message':'%s'}",errorMessage);
        out = buffer;
    }
    return 0;
}


int GetSetting(const char* host_str,string devAddr, string& out)
{
    //string out;
    //BYTE addr = 56; 
    //const char* host_str = "172.17.2.75";

    char buffer[1024];
    char errorMessage[1024];
    unsigned short post = 19090;

    dllprivate* lpUnitCfg = new dllprivate;
    lpUnitCfg->clearConfig();
    lpUnitCfg = StartMonitor(host_str,post);

    const char* str = devAddr.data();
    int intStr = atoi(str);
    BYTE addr = intStr;
	int ret = requestTitle(lpUnitCfg, 0, 1);
	if(ret != COMM_OK)	
	{
		sprintf(errorMessage,"������վ�����ʧ��...\n");
		return ret;
	}

    GroupBase* pGroup = lpUnitCfg->findGroup(0, 1, GroupBase::GROUP_TYPE_DEVICE_CONFIG);
    if (pGroup == NULL)
    {
        sprintf(errorMessage,"��վ����Ϊ��...\n");
        return ret;
    }
	ret = requestGroupInfos(lpUnitCfg, 0, 1, pGroup->groupNo);
	if(ret != COMM_OK)
	{
		sprintf(errorMessage,"������վ�豸ʧ��...\n");
		return ret;
	}

    ret = requestDevice(lpUnitCfg,addr);
	if(ret == COMM_OK)
		printf("�����������");
	else
	{
		sprintf(errorMessage,"��������ʧ��\n");
		return ret;
	}

    for(GroupList::const_iterator pDevice = lpUnitCfg->groupList.begin();
        pDevice != lpUnitCfg->groupList.end();++pDevice)
    {
        GroupBase* gb = *pDevice;
        switch(gb->GroupType())
        {
        case GroupBase::GROUP_TYPE_DEVICE_CONFIG:
            {
                for(ItemList::const_iterator itemIter = gb->m_ItemList.begin();itemIter != gb->m_ItemList.end();++itemIter)
                {
                    DeviceItem* dev  = (DeviceItem*)itemIter->second;
                    if(dev->itemNo == 0)
                    {
                        continue;
                    }
                    for(vector<CpuItem>::const_iterator cpuIter = dev->cpus.begin();cpuIter != dev->cpus.end();++cpuIter)
                    {
                        for(GroupList::const_iterator it = cpuIter->m_groups.begin();
		                    it != cpuIter->m_groups.end(); it++)
                        {
                            if((*it)->GroupType() == GroupBase::GROUP_TYPE_SETTING
                                || (*it)->GroupType() == GroupBase::GROUP_TYPE_SETTING_AREA)
                            {
                                requestGroupInfos(lpUnitCfg, addr, cpuIter->cpuno, (*it)->groupNo);
                                ret = requestGroup(lpUnitCfg, addr, cpuIter->cpuno, (*it)->groupNo, STD103_KOD_GETDATA);
                            }
                        }
                    }
                }
            }
        }
    }
    bool cpuNum = false;//ֻҪһ��cpu����,������Ϊ�õ����ݱ�־
    DeviceItem* pDevice =lpUnitCfg->getDevice(addr);
    if(pDevice != NULL)
    {
        for(vector<CpuItem>::const_iterator cpuIter = pDevice->cpus.begin();cpuIter != pDevice->cpus.end();++cpuIter)
        {
            if(!cpuNum)
            {
                sprintf(buffer,"{'result':1, 'message':'', 'content':[");
                out += buffer;
                char* s1 = "��ֵ";
                char* s2 = "��ѹ��";
                char* s3 = "��ֵ";
                for(vector<GroupBase*>::const_iterator GroupIter = cpuIter->m_groups.begin();GroupIter != cpuIter->m_groups.end();++GroupIter)
                {
                    char* s0 = (*GroupIter)->groupDesc;
                    if(NULL != strstr(s0,s1)||
                        NULL != strstr(s0,s2)||
                        NULL != strstr(s0,s3))
                    {
                        unsigned char gNo = (*GroupIter)->groupNo;
                        sprintf(buffer,"{'gno':%d, 'gdesc':'%s', 'item':[\n",(*GroupIter)->groupNo,(*GroupIter)->groupDesc);
                        out += buffer;
                        for (ItemList::const_iterator valueItem = (*GroupIter)->m_ItemList.begin();
                            valueItem != (*GroupIter)->m_ItemList.end(); valueItem++)
                        {
                            SettingBase* pd = (SettingBase*)valueItem->second;
                            if (pd->setType == SettingBase::GROUP_TYPE_SETTINGINT)
                            {                                    
                                sprintf(buffer,"{'ino':%d, 'name':'%s', 'value':%d\n},",valueItem->second->itemNo,(valueItem->second->itemDesc),pd->i.value);
                                out += buffer;
                            }
	                        else if (pd->setType == SettingBase::GROUP_TYPE_SETTINGFLOAT)
                            {
                                sprintf(buffer,"{'ino':%d, 'name':'%s', 'value':%f}\n,",valueItem->second->itemNo,(valueItem->second->itemDesc),pd->f.value);
                                out += buffer;
                            }
                            else if (pd->setType == SettingBase::GROUP_TYPE_SETTINGCTRLWORD)
                            {                                   
                                sprintf(buffer,"{'ino':%d, 'name':'%s', 'value':%d}\n,",valueItem->second->itemNo,(valueItem->second->itemDesc),pd->c.value);
                                out += buffer;
                            }
                            else if (pd->setType == SettingBase::GROUP_TYPE_SETTINGDESC)
                            { 
                                sprintf(buffer,"{'ino':%d, 'name':'%s', 'value':'%s'}\n,",valueItem->second->itemNo,(valueItem->second->itemDesc),pd->d.value);
                                out += buffer;
                            }
                        }
                        out = out.substr(0,out.length()-1);
                        sprintf(buffer,"]}\n,");
                        out += buffer;
                    }
                    else continue;
                }
                out = out.substr(0,out.length()-1);
                sprintf(buffer,"]}\n");
                out += buffer;
                cpuNum = true;
            }
        }
    }
    if(true==cpuNum)
    {
        const char * p= out.data();
        //printf("%s",p);
    }
    else
    {
        sprintf(buffer,"{'result':0, 'message':'%s'}",errorMessage);
        out = buffer;
    }
	return 1;
}