
def read_clean_and_append(file_to_read, file_to_append):
    with open(file_to_read, 'r', encoding='utf-16-le') as f:
        lines = f.readlines()
        fa = open(file_to_append, 'a', encoding='utf-8')

        for line in lines:
            line = line.strip()
            line = line.replace("Temperature", "")
            line = line.replace("VpH", "")
            line = line.replace("┬░ ", ",")
            line = line.replace("%", "")
            line = line.replace("‘", "")
            line = line.replace(":", "")
            line = line.replace("V", "")
            line = line.replace("\t", "")
            #print(line)
            fa.write(line.strip() + '\n')

        fa.close()


read_clean_and_append('../ph9_1.txt', '../ph9.csv')
