#include <stdio.h>
#include <string.h>

/*
Kiểm tra dấu chấm trong địa chỉ ip
Output: 1 - dấu chấm hợp lệ, 0 - dấu chấm lỗi
*/
int check_period(char *string)
{
  int count_period = 0, n = strlen(string);

  if (string[0] == '.')
    return 0;
  if (string[n - 1] == '.')
    return 0;
  for (int i = 0; i < n - 1; i++)
  {
    if (string[i] == '.')
      count_period++;
    if (string[i] == '.' && string[i + 1] == '.') //Kiểm tra 2 dấu chấm có cạnh nhau không
      return 0;
  }
  if (count_period != 3) //Số lượng dấu chấm khác 3 sẽ fail
    return 0;
  return 1;
}

/*
Kiểm tra xem string có là địa chỉ IP không
Output: 1 - là địa chỉ IP, 0 - không là địa chỉ IP
*/
int check_IP(char *string)
{
  int value = 0, n = strlen(string);
  if (check_period(string) == 0)
  {
    return 0;
  }
  else
  {
    for (int i = 0; i < n; i++)
    {
      if (string[i] == '.')
      {
        if (value < 0 || value > 255)
          return 0;
        value = 0;
      }
      else
      {
        if (string[i] >= '0' && string[i] <= '9')
        {
          value = value * 10 + (string[i] - '0');
          if (i == n - 1)
            if (value < 0 || value > 255)
              return 0;
        }
        else
          return 0;
      }
    }
    return 1;
  }
}

/*
Kiểm tra số hiệu cổng
Output: 1 - cổng hợp lệ, 0 - cổng không hợp lệ
*/
int check_port(char *port)
{
  int n = strlen(port);
  for (int i = 0; i < n; i++)
  {
    if (port[i] < '0' || port[i] > '9')
      return 0;
  }
  return 1;
}