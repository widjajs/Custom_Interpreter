with open("test.txt", "w") as file:
    for i in range(260):
        file.write(f"let a{i} = {i};\n")
    file.write("print a0;\nprint a128\nprint a259;")
