#ifndef __CONFIG_H_INCLUDE__
#define __CONFIG_H_INCLUDE__

#include <msgpack.hpp>

struct msg_user_currency_info_t
{
    MSGPACK_DEFINE(m_iGold, m_iEnergy);

    int m_iGold;
    int m_iEnergy;
};

struct msg_user_info_t
{
    MSGPACK_DEFINE(m_stCurrencyInfo, m_iUin, m_iGroupID, m_iLevel);

    msg_user_currency_info_t m_stCurrencyInfo;

    int m_iUin;
    int m_iGroupID;
    int m_iLevel;
};

typedef std::map<std::string, int> msg_user_status_t;

struct msg_all_user_info_t
{
    MSGPACK_DEFINE(m_stUserInfo, m_stStatus);

    msg_user_info_t     m_stUserInfo;
    msg_user_status_t   m_stStatus;
};

//////////////////////////////////////////////////////////////
/////////  Month Card Config

struct msg_month_card_info_elem_t
{
    MSGPACK_DEFINE(m_strName, m_strIcon, m_strDesc, m_iCost);

    std::string     m_strName;
    std::string     m_strIcon;
    std::string     m_strDesc;
    int             m_iCost;
};

typedef std::map<int, msg_month_card_info_elem_t> msg_month_card_info_t;

#endif // __CONFIG_H_INCLUDE__
