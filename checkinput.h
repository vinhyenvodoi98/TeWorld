/* 
Kiểm tra dấu chấm trong địa chỉ ip
Output: 1 - dấu chấm hợp lệ, 0 - dấu chấm lỗi
*/
int check_period(char *string);

/* 
Kiểm tra xem string có là địa chỉ IP không
Output: 1 - là địa chỉ IP, 0 - không là địa chỉ IP
*/
int check_IP(char *string);

/*
Kiểm tra số hiệu cổng
Output: 1 - cổng hợp lệ, 0 - cổng không hợp lệ
*/
int check_port(char *port);