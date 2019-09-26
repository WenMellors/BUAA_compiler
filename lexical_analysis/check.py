import filecmp

test = open('output.txt', 'r')
anwser = open('anwser.txt', 'r')

print(filecmp.cmp('output.txt', 'anwser.txt', False))