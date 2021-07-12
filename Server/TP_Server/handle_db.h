#pragma once

//
// DB 코드는 MSDN의 예제 코드 참고 및 수정하여 작성.
//

#include <sqlext.h>  
void DBErrorDisplay(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
 
bool SearchUserID(string name, int id);
bool InsertUserData(int id);
bool SaveUserData(int id);

// DB 접근도 블럭킹 연산
// 별도의 타이머를 두고 작업하도록 해야
// DB 작업으로 메인 작업 쓰레드가 멈추지 않는다 
// 현재 코드에는 반영이 안되어있음 ,ㅅ,
void AddDBEvent(int id, OP_TYPE type);
void DoDBTimer();