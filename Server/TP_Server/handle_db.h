#pragma once

//
// DB �ڵ�� MSDN�� ���� �ڵ� ���� �� �����Ͽ� �ۼ�.
//

#include <sqlext.h>  
void DBErrorDisplay(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
 
bool SearchUserID(string name, int id);
bool InsertUserData(int id);
bool SaveUserData(int id);

// DB ���ٵ� ��ŷ ����
// ������ Ÿ�̸Ӹ� �ΰ� �۾��ϵ��� �ؾ�
// DB �۾����� ���� �۾� �����尡 ������ �ʴ´� 
// ���� �ڵ忡�� �ݿ��� �ȵǾ����� ,��,
void AddDBEvent(int id, OP_TYPE type);
void DoDBTimer();