my = open("output.txt", "r", encoding="utf8").readlines()
room = open("output1.txt", "r", encoding="utf8").readlines()

i = 0
while i < len(my):
    if my[i] != room[i]:
        print(i)
        break
    i += 1
if len(my) != len(room):
  print("len error")
else :
  print("success")