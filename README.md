# signals
C++ signal/slot

使用:
1.定义signal:  
    plk::signal<R(PARAMT1, PARAMT2, ...)> mysignal;

2.触发signal:  
    mysignal.emit(param1, param2, ...); 或者 mysignal(param1, param2, ...);

3.注册slot:  
    auto conn = mysignal.attach(my_slot_function_or_object); 
    (若不用注销，则不必保存返回的conn.)

4.注销slot:  
    conn.detach();
