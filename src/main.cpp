#include <fstream>
#include <iostream>
#include <streambuf>  

#include <msgpack.hpp>

#include "config.h"

#include "json2class.hpp"

int main(void)
{
    {
        std::ifstream ifs("in");  
        std::string str((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());

        msg_all_user_info_t stUserInfo;

        try
        {
            DecodeJsonPack(str, stUserInfo);

            std::cout << "Gold: " << stUserInfo.m_stUserInfo.m_stCurrencyInfo.m_iGold << std::endl;
            std::cout << "Energy: " << stUserInfo.m_stUserInfo.m_stCurrencyInfo.m_iEnergy << std::endl;
            std::cout << "Uin: " << stUserInfo.m_stUserInfo.m_iUin << std::endl;
            std::cout << "GroupID: " << stUserInfo.m_stUserInfo.m_iGroupID << std::endl;

            std::cout << "Msg: " << stUserInfo.m_stStatus["msg"] << std::endl;
            std::cout << "Ret: " << stUserInfo.m_stStatus["ret"] << std::endl;
        }
        catch (const std::bad_cast& stEx) 
        {
            std::cout << stEx.what() << std::endl;
        }
    }

    {
        std::ifstream ifs("in2");  
        std::string str((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());

        msg_month_card_info_t stMonthCardInfo;

        try
        {
            DecodeJsonPack(str, stMonthCardInfo);
            std::cout << stMonthCardInfo[4001].m_strName << ", "
                      << stMonthCardInfo[4001].m_strIcon << ", "
                      << stMonthCardInfo[4001].m_strDesc << ", "
                      << stMonthCardInfo[4001].m_iCost << std::endl;

            std::cout << stMonthCardInfo[4002].m_strName << ", "
                      << stMonthCardInfo[4002].m_strIcon << ", "
                      << stMonthCardInfo[4002].m_strDesc << ", "
                      << stMonthCardInfo[4002].m_iCost << std::endl;
        }
        catch (const std::bad_cast& stEx) 
        {
            std::cout << stEx.what() << std::endl;
        }
    }

    return 0;
}
