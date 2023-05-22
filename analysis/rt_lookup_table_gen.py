import csv

def generate_header_file(input_csv, output_header, min_temp, max_temp):
    with open(input_csv, 'r') as csv_file:
        reader = csv.reader(csv_file)
        rows = list(reader)[1:]     # skip the header row
    
    # check the CSV structure
    if len(rows[0]) != 2:
        raise ValueError("The CSV file must contain exactly two columns (temperature, resistance).")

    # filter data based on the given temperature range
    rows = [row for row in rows if min_temp <= float(row[0]) <= max_temp]

    # define the table in the header file
    header_content = "#ifndef RT_TABLE_H\n#define RT_TABLE_H\n\n"
    header_content += "const int RT_TABLE_SIZE = {};\n\n".format(len(rows))
    header_content += "const float RT_TABLE[] = {\n"

    for row in rows:
        temp, res = row
        # format: {resistance, temperature},
        header_content += "    {" + res + ", " + temp + "},\n"
    
    header_content += "};\n\n#endif // RT_TABLE_H\n"

    # write the header file
    with open(output_header, 'w') as header_file:
        header_file.write(header_content)

# usage
generate_header_file('103JG1F_RT_Table.csv', 'RT_Table.h', 10, 220)
