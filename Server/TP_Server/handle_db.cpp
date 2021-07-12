#include "stdafx.h"
#include "handle_db.h"

/************************************************************************
/* DBErrorDisplay : display error/warning information
/*
/* Parameters:
/* hHandle ODBC handle
/* hType Type of handle (SQL_HANDLE_STMT, SQL_HANDLE_ENV, SQL_HANDLE_DBC)
/* RetCode Return code of failing command
/************************************************************************/ 
constexpr int NAME_LEN = 30;
 
extern mutex	g_sector_locker;

extern array <ServerObject*, MAX_USER + 1> objects;
extern queue<DB_EVENT> db_timer_queue;
extern mutex db_timer_lock;
extern HANDLE h_iocp;
 
extern MAP_TILE_DATA g_TileDatas[WORLD_HEIGHT][WORLD_WIDTH];
bool SearchUserID(string name, int id)
{
    SQLHENV henv;
    SQLHDBC hdbc; 
    bool findRes = false; 
    SQLHSTMT hstmt = 0;
    SQLWCHAR user_name[NAME_LEN];
    SQLINTEGER user_x, user_y, user_level, user_hp, user_exp, user_gold,
        user_item1, user_item2, user_item3, user_item4;
    SQLLEN cb_user_name = 0, cb_user_x = 0, cb_user_y = 0, cb_user_level = 0,
        cb_user_hp = 0, cb_user_exp = 0, cb_user_gold = 0,
        cb_user_item1, cb_user_item2, cb_user_item3, cb_user_item4;
    wchar_t buf[128] = {};
    SQLRETURN retcode;
    // Allocate environment handle  
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

    // Set the ODBC version environment attribute  
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

        // Allocate connection handle  
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

            // Set login timeout to 5 seconds  
            if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
                // Connect to data source  
                retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2021_gs_odbc", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

                // Allocate statement handle  
                if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

                    string query = "EXEC select_user_id " + name;
                    wstring wQuery(query.begin(), query.end());
                    retcode = SQLExecDirect(hstmt, (SQLWCHAR*)wQuery.c_str(), SQL_NTS);
                    if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
                        DBErrorDisplay(hstmt, SQL_HANDLE_STMT, retcode);
                    }
                    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                        // Bind columns 
                        retcode = SQLBindCol(hstmt, 1, SQL_C_WCHAR, &user_name, NAME_LEN, &cb_user_name);
                        retcode = SQLBindCol(hstmt, 2, SQL_C_ULONG, &user_level, 10, &cb_user_level);
                        retcode = SQLBindCol(hstmt, 3, SQL_C_ULONG, &user_hp, 10, &cb_user_hp);
                        retcode = SQLBindCol(hstmt, 4, SQL_C_ULONG, &user_exp, 10, &cb_user_exp);
                        retcode = SQLBindCol(hstmt, 5, SQL_C_ULONG, &user_gold, 10, &cb_user_gold); 

                        retcode = SQLBindCol(hstmt, 6, SQL_C_ULONG, &user_x, 10, &cb_user_x);
                        retcode = SQLBindCol(hstmt, 7, SQL_C_ULONG, &user_y, 10, &cb_user_y);

                        retcode = SQLBindCol(hstmt, 8, SQL_C_ULONG, &user_item1, 10, &cb_user_item1);
                        retcode = SQLBindCol(hstmt, 9, SQL_C_ULONG, &user_item2, 10, &cb_user_item2);
                        retcode = SQLBindCol(hstmt, 10, SQL_C_ULONG, &user_item3, 10, &cb_user_item3);
                        retcode = SQLBindCol(hstmt, 11, SQL_C_ULONG, &user_item4, 10, &cb_user_item4);
                        // Fetch and print each row of data. On an error, display a message and exit.  

                        retcode = SQLFetch(hstmt);
                        if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) { 
                            DBErrorDisplay(hstmt, SQL_HANDLE_STMT, retcode); 
                        }
                        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
                        {
                            //cout << "등록된 ID - 기존 플레이어!\n";
                            findRes = true; 
                            objects[id]->LEVEL = user_level;
                            objects[id]->HP = user_hp;
                            objects[id]->EXP = user_exp;
                            objects[id]->GOLD = user_gold;
                            objects[id]->start_x = objects[id]->x = rand() % 2000;
                            objects[id]->start_y = objects[id]->y = rand() % 2000;
                            //objects[id]->x = rand() % 2000;
                           // objects[id]->y = rand() % 2000;

                            objects[id]->itemCount = 0;
                            if (user_item1 != 0) {
                                objects[id]->Items.push_back((ITEM_TYPE)user_item1);
                                objects[id]->itemCount++;
                            }
                            if (user_item2 != 0) {
                                objects[id]->Items.push_back((ITEM_TYPE)user_item2);
                                objects[id]->itemCount++;
                            }
                            if (user_item3 != 0) {
                                objects[id]->Items.push_back((ITEM_TYPE)user_item3);
                                objects[id]->itemCount++;
                            }
                            if (user_item4 != 0) {
                                objects[id]->Items.push_back((ITEM_TYPE)user_item4);
                                objects[id]->itemCount++;
                            }
                        }
                        else {
                            findRes = true;
                            //cout << "등록되지 않은 ID - 신규 플레이어!\n";
                            objects[id]->x = rand() % 2000;
                            objects[id]->y = rand() % 2000;
                            objects[id]->start_x = objects[id]->x;
                            objects[id]->start_y = objects[id]->y;

                            objects[id]->LEVEL = 1;

                            InsertUserData(id);
                        }
                    } 
                    // Process data  
                    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                        SQLCancel(hstmt);
                        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                    }

                    SQLDisconnect(hdbc);
                }
                SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
            }
        }
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
    }

    return findRes;
}
bool InsertUserData(int id)
{
    SQLHENV henv;
    SQLHDBC hdbc; 
    bool findRes = false;
    SQLHSTMT hstmt = 0;
    SQLWCHAR user_name[NAME_LEN];
    SQLINTEGER user_x, user_y, user_level, user_hp, user_exp, user_gold,
        user_item1, user_item2, user_item3, user_item4;
    SQLLEN cb_user_name = 0, cb_user_x = 0, cb_user_y = 0, cb_user_level = 0,
        cb_user_hp = 0, cb_user_exp = 0, cb_user_gold = 0,
        cb_user_item1, cb_user_item2, cb_user_item3, cb_user_item4 ;
    wchar_t buf[128] = {};
    SQLRETURN retcode;
    // Allocate environment handle  
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

    // Set the ODBC version environment attribute  
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

        // Allocate connection handle  
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

            // Set login timeout to 5 seconds  
            if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
                // Connect to data source  
                retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2021_gs_odbc", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

                // Allocate statement handle  
                if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

                    //@ParamName nchar(20), @ParamLevel int, @ParamHp int, @ParamExp int, @ParamGold int

                    string query = "EXEC add_user " + string(objects[id]->m_name) + ", " +
                        to_string(objects[id]->LEVEL) + ", " +
                        to_string(objects[id]->HP) + ", " +
                        to_string(objects[id]->EXP) + ", " +
                        to_string(objects[id]->GOLD) + ", " +
                        to_string(objects[id]->x) + ", " + 
                        to_string(objects[id]->y);

                    wstring wQuery(query.begin(), query.end());
                    retcode = SQLExecDirect(hstmt, (SQLWCHAR*)wQuery.c_str(), SQL_NTS);
                    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                        // Bind columns 
                        // retcode = SQLBindCol(hstmt, 1, SQL_C_WCHAR, &user_name, NAME_LEN, &cb_user_name);
                        // retcode = SQLBindCol(hstmt, 2, SQL_C_ULONG, &user_level, 10, &cb_user_level);
                        // retcode = SQLBindCol(hstmt, 3, SQL_C_ULONG, &user_hp, 10, &cb_user_hp);
                        // retcode = SQLBindCol(hstmt, 4, SQL_C_ULONG, &user_exp, 10, &cb_user_exp);
                        // retcode = SQLBindCol(hstmt, 5, SQL_C_ULONG, &user_gold, 10, &cb_user_gold);

                        // Fetch and print each row of data. On an error, display a message and exit.  

                        retcode = SQLFetch(hstmt);
                        if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
                            DBErrorDisplay(hstmt, SQL_HANDLE_STMT, retcode);
                        }
                        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
                        {
                           // cout << "등록 성공!!!\n"; 
                        }
                        else {
                            findRes = true;
                            cout << "등록 실패...\n"; 
                        }
                    } 
                    else{
                        DBErrorDisplay(hstmt, SQL_HANDLE_STMT, retcode);
                    }
                    // Process data  
                    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                        SQLCancel(hstmt);
                        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                    }

                    SQLDisconnect(hdbc);
                }
                SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
            }
        }
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
    }

    return findRes;
}
bool SaveUserData(int id)
{
    SQLHENV henv;
    SQLHDBC hdbc; 
    bool findRes = false;
    SQLHSTMT hstmt = 0;
    SQLWCHAR user_name[NAME_LEN];
    SQLINTEGER user_x, user_y, user_level, user_hp, user_exp, user_gold,
        user_item1, user_item2, user_item3, user_item4;
    SQLLEN cb_user_name = 0, cb_user_x = 0, cb_user_y = 0, cb_user_level = 0,
        cb_user_hp = 0, cb_user_exp = 0, cb_user_gold = 0,
        cb_user_item1, cb_user_item2, cb_user_item3, cb_user_item4;
    wchar_t buf[128] = {};
    SQLRETURN retcode;
    // Allocate environment handle   
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

    // Set the ODBC version environment attribute  
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

        // Allocate connection handle  
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

            // Set login timeout to 5 seconds  
            if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
                // Connect to data source  
                retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2021_gs_odbc", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

                // Allocate statement handle  
                if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

                    //@ParamName nchar(20), @ParamLevel int, @ParamHp int, @ParamExp int, @ParamGold int

                    string query = "EXEC save_user_data " + string(objects[id]->m_name) + ", " +
                        to_string(objects[id]->LEVEL) + ", " +
                        to_string(objects[id]->HP) + ", " +
                        to_string(objects[id]->EXP) + ", " +
                        to_string(objects[id]->GOLD) + ", " +
                        to_string(objects[id]->x) + ", " +
                        to_string(objects[id]->y); 

                    for (int i = 0; i < objects[id]->itemCount; ++i) { 
                        query += + ", " + to_string(objects[id]->Items[i]);
                    }
                    for (int i = objects[id]->itemCount; i < 4; ++i) {
                        query += + ", 0";
                    }
                    wstring wQuery(query.begin(), query.end());
                    retcode = SQLExecDirect(hstmt, (SQLWCHAR*)wQuery.c_str(), SQL_NTS);
                    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                        // Bind columns 
                        // retcode = SQLBindCol(hstmt, 1, SQL_C_WCHAR, &user_name, NAME_LEN, &cb_user_name);
                        // retcode = SQLBindCol(hstmt, 2, SQL_C_ULONG, &user_level, 10, &cb_user_level);
                        // retcode = SQLBindCol(hstmt, 3, SQL_C_ULONG, &user_hp, 10, &cb_user_hp);
                        // retcode = SQLBindCol(hstmt, 4, SQL_C_ULONG, &user_exp, 10, &cb_user_exp);
                        // retcode = SQLBindCol(hstmt, 5, SQL_C_ULONG, &user_gold, 10, &cb_user_gold);

                        // Fetch and print each row of data. On an error, display a message and exit.  

                        //retcode = SQLFetch(hstmt);
                        if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
                            DBErrorDisplay(hstmt, SQL_HANDLE_STMT, retcode);
                        }
                        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
                        {
                            //cout << "갱신 성공!!!\n";
                        }
                        else {
                            findRes = true;
                            cout << "갱신 실패...\n";
                        }
                    }
                    else {
                        DBErrorDisplay(hstmt, SQL_HANDLE_STMT, retcode);
                    }
                    // Process data  
                    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                        SQLCancel(hstmt);
                        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                    }

                    SQLDisconnect(hdbc);
                }
                SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
            }
        }
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
    } 
    return findRes;
}
void DBErrorDisplay(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
    SQLSMALLINT iRec = 0;
    SQLINTEGER iError;
    WCHAR wszMessage[1000];
    WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
    if (RetCode == SQL_INVALID_HANDLE) {
        fwprintf(stderr, L"Invalid handle!\n");
        return;
    }
    while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
        (SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
        // Hide data truncated..
        if (wcsncmp(wszState, L"01004", 5)) {
            fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
        }
    }
}
void AddDBEvent(int id, OP_TYPE type)
{
    db_timer_lock.lock();
    db_timer_queue.emplace(DB_EVENT{ id, type });
    db_timer_lock.unlock();
} 

void DoDBTimer()
{
    while (true) {
        this_thread::sleep_for(10ms);

        db_timer_lock.lock();
        if (true == db_timer_queue.empty()) {
            db_timer_lock.unlock();
            this_thread::sleep_for(10ms);
        }
        else {
            DB_EVENT ev = db_timer_queue.front();

            db_timer_queue.pop();
            db_timer_lock.unlock();

            EX_OVER* ex_over = new EX_OVER;
            ex_over->m_op = ev.type;
            PostQueuedCompletionStatus(h_iocp, 1, ev.id, &ex_over->m_over);
        } 
    }
}