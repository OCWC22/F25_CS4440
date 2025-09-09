def compress_file(input_file, output_file):
    with open(input_file, 'r') as source, open(output_file, 'w') as dest:
        count = 0
        previous_char = None
        
        for char in source.read():
            if char in [' ', '\n']:
                # Handle spaces/newlines
                if count >= 16 and previous_char:
                    if previous_char == '1':
                        dest.write(f"+{count}+")
                    else:
                        dest.write(f"-{count}-")
                elif previous_char:
                    dest.write(previous_char * count)
                
                dest.write(char)  # Write the space/newline
                count = 0
                previous_char = None
                continue
                
            if previous_char is None:
                previous_char = char
                count = 1
            elif char == previous_char:
                count += 1
            else:
                # Character changed, write previous run
                if count >= 16:
                    if previous_char == '1':
                        dest.write(f"+{count}+")
                    else:
                        dest.write(f"-{count}-")
                else:
                    dest.write(previous_char * count)
                
                previous_char = char
                count = 1
        
        # Handle any remaining characters at end
        if count >= 16 and previous_char:
            if previous_char == '1':
                dest.write(f"+{count}+")
            else:
                dest.write(f"-{count}-")
        elif previous_char:
            dest.write(previous_char * count)