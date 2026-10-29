//readelf -h abc
//0x56559180


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char debug_mode = 0;             // 0 = off, 1 = on
char file_name[128] = "";        // Stores the targeted file name
int unit_size = 1;               // Default unit size is 1 byte
unsigned char mem_buf[10000];    // Buffer to hold file data later
size_t mem_count = 0;            // Number of bytes currently loaded
char display_mode = 0; // 0 = Hexadecimal (Initial state), 1 = Decimal

struct fun_desc {
    char *name;
    char index;                  // Added to match Lab 1 scheme mapping
    void (*fun)();
};

// Format string arrays placed globally
static char* hex_formats[] = {"%02hhX\n", "%04hX\n", "No such unit", "%08x\n"};
static char* dec_formats[] = {"%hhd\n", "%hd\n", "No such unit", "%d\n"};

// Function prototypes
void toggle_debug();
void set_file_name();
void set_unit_size();
void load_into_memory();   
void toggle_display();     
void memory_display();     
void save_into_file();     
void memory_modify();      
void quit();

struct fun_desc menu[] = {
    {"Toggle Debug Mode", 'D', toggle_debug},
    {"Set File Name", 'F', set_file_name},
    {"Set Unit Size", 'U', set_unit_size},
    {"Load Into Memory", 'L', load_into_memory},
    {"Toggle Display Mode", 'T', toggle_display},
    {"Memory Display", 'M', memory_display},
    {"Save Into File", 'S', save_into_file},
    {"Memory Modif(y)", 'Y', memory_modify},
    {"Quit", 'Q', quit},
    {NULL, 0, NULL}
};

int main(int argc, char **argv) {
    char input[10];

    while (1) {
        // Debug status printed to stderr right before the menu
        if (debug_mode) {
            fprintf(stderr, "Debug: unit_size=%d, file_name='%s', mem_count=%zu\n", 
                    unit_size, file_name, mem_count);
        }

        printf("Choose action:\n");
        int i = 0;
        while (menu[i].name != NULL) {
            printf("%c - %s\n", menu[i].index, menu[i].name);
            i++;
        }

        if (fgets(input, sizeof(input), stdin) == NULL) break;
        
        char choice = input[0];
        int found = 0;

        i = 0;
        while (menu[i].name != NULL) {
            if (menu[i].index == toupper((unsigned char)choice)) {
                menu[i].fun();
                found = 1;
                break;
            }
            i++;
        }

        if (!found) {
            printf("Invalid choice\n");
        }
        printf("\n");
    }
    return 0;
}

void toggle_debug() {
    if (debug_mode == 0) {
        debug_mode = 1;
        fprintf(stderr, "Debug flag now on\n");
    } else {
        debug_mode = 0;
        fprintf(stderr, "Debug flag now off\n");
    }
}

void set_file_name() {
    printf("Enter file name: ");
    if (fgets(file_name, sizeof(file_name), stdin) != NULL) {
        file_name[strcspn(file_name, "\n")] = 0;        // Remove trailing newline
        
        if (debug_mode) {
            fprintf(stderr, "Debug: file name set to '%s'\n", file_name);
        }
    }
}

void set_unit_size() {
    char input[10];
    printf("Enter unit size (1, 2, or 4): ");
    if (fgets(input, sizeof(input), stdin) != NULL) {
        int size = atoi(input);
        if (size == 1 || size == 2 || size == 4) {
            unit_size = size;
            if (debug_mode) {
                fprintf(stderr, "Debug: set size to %d\n", unit_size);
            }
        } else {
            printf("Error: Invalid unit size.\n");
        }
    }
}

void quit() {
    if (debug_mode) {
        fprintf(stderr, "quitting\n");
    }
    exit(0);
}

void load_into_memory() { 
    // Check if file_name is empty
    if (strcmp(file_name, "") == 0) {
        printf("Error: No file name set.\n");
        return;
    }

    //Open file_name for reading in binary mode
    FILE *file = fopen(file_name, "rb");
    if (!file) {
        perror("Error opening file");
        return;
    }
    
    char input[100];
    unsigned int location;
    int length;

    // Prompt the user for location (hex) and length (dec)
    printf("Enter <location> <length>: ");
    if (fgets(input, sizeof(input), stdin) == NULL) {
        fclose(file);
        return;
    }
    // Parse location as hex (%x) and length as decimal (%d)
    if (sscanf(input, "%x %d", &location, &length) != 2) {
        printf("Error: Invalid input format.\n");
        fclose(file);
        return;
    }
    if (debug_mode) {
        fprintf(stderr, "Debug: file_name='%s', location=0x%x, length=%d\n", 
                file_name, location, length);
    }
    // Seek to the requested file location
    if (fseek(file, location, SEEK_SET) != 0) {
        printf("Error: Failed to seek to location 0x%x\n", location);
        fclose(file);
        return;
    }
    //copy (length * unit_size) bytes into mem_buf
    size_t bytes_to_read = length * unit_size;

    // buffer overflow check
    if (bytes_to_read > 10000) {
        printf("Error: Requested size exceeds maximum buffer limit (10000 bytes).\n");
        fclose(file);
        return;
    }

    size_t items_read = fread(mem_buf, unit_size, length, file);
    // update global memory tracking
    mem_count = items_read * unit_size;

    printf("Loaded %zu units into memory\n", items_read);
    
    fclose(file);
}
void toggle_display()   {
   if (display_mode == 0) {
        display_mode = 1;
        printf("Decimal display flag now on, decimal representation\n");
    } else {
        display_mode = 0;
        printf("Decimal display flag now off, hexadecimal representation\n");
    } 
}
void memory_display() {
    char input[100];
    unsigned int address;
    int length;

    // Prompt the user for input
    printf("Enter address and length\n");
    if (fgets(input, sizeof(input), stdin) == NULL) return;

    // Parse address (hexadecimal) and length/unit count (decimal)
    if (sscanf(input, "%x %d", &address, &length) != 2) {
        printf("Error: Invalid input format.\n");
        return;
    }

    if (debug_mode) {
        fprintf(stderr, "Debug: Displaying %d units from address 0x%x\n", length, address);
    }

    // Special case: If address is 0, start from the beginning of mem_buf
    unsigned char* ptr = (address == 0) ? mem_buf : (unsigned char*)address;
    
    // Print the header matching the display flag status
    if (display_mode == 0) {
        printf("Hexadecimal\n===========\n");
    } else {
        printf("Decimal\n=======\n");
    }
    
    // Iterate and print each unit safely based on its specified width
    int i;
    for (i = 0; i < length; i++) {
        unsigned int var = 0;

        if (unit_size == 1) {
            var = *(ptr + i * 1);
        } else if (unit_size == 2) {
            // Cast pointer to extract 2 bytes at a time
            var = *((unsigned short*)(ptr + i * 2));
        } else if (unit_size == 4) {
            // Cast pointer to extract 4 bytes at a time
            var = *((unsigned int*)(ptr + i * 4));
        }

        // Print using the selected display mode format array
        if (display_mode == 0) { // Hexadecimal
            printf(hex_formats[unit_size - 1], var);
        } else { // Decimal
            printf(dec_formats[unit_size - 1], var);
        }
    }
}

void save_into_file()   { 
    // Check if file_name is set
    if (strcmp(file_name, "") == 0) {
        printf("Error: File name is not set.\n");
        return;
    }

    // Open file_name in read/write binary mode to avoid truncating it
    FILE *file = fopen(file_name, "r+b");
    if (!file) {
        perror("Error opening file for modifications");
        return;
    }

    // Prompt user for mixed-base inputs
    char input[100];
    unsigned int source_address;
    unsigned int target_location;
    int length;

    printf("Please enter <source-address> <target-location> <length>\n");
    if (fgets(input, sizeof(input), stdin) == NULL) {
        fclose(file);
        return;
    }

    // Parse source (hex), target offset (hex), and unit length (dec)
    if (sscanf(input, "%x %x %d", &source_address, &target_location, &length) != 3) {
        printf("Error: Invalid input format.\n");
        fclose(file);
        return;
    }

    if (debug_mode) {
        fprintf(stderr, "Debug: source_address=0x%x, target_location=0x%x, length=%d\n", 
                source_address, target_location, length);
    }

    // Boundary Check: Determine file size to prevent writing past or out of bounds
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);

    if ((long)target_location > file_size) {
        printf("Error: Target location (0x%x) exceeds file boundaries (size: %ld bytes).\n", 
               target_location, file_size);
        fclose(file);
        return;
    }

    // Seek to the designated target offset location
    if (fseek(file, target_location, SEEK_SET) != 0) {
        printf("Error: Failed seeking to target location 0x%x\n", target_location);
        fclose(file);
        return;
    }

    // Source selection: Handle special case 0 -> mem_buf
    unsigned char* src_ptr = (source_address == 0) ? mem_buf : (unsigned char*)source_address;

    // Write the specified number of units to disk
    size_t bytes_to_write = length * unit_size;
    size_t items_written = fwrite(src_ptr, unit_size, length, file);

    if (items_written != (size_t)length) {
        printf("Error: Written units mismatch (%zu written out of %d requested).\n", 
               items_written, length);
    } else {
        printf("Successfully modified %zu bytes at offset 0x%x\n", bytes_to_write, target_location);
    }

    fclose(file);
}
void memory_modify()    {

    char input[100];
    unsigned int location;
    unsigned int val;

    // Prompt user for input (both parameters in hexadecimal)
    printf("Please enter <location> <val>\n");
    if (fgets(input, sizeof(input), stdin) == NULL) return;

    // Parse location (hexadecimal) and val (hexadecimal)
    if (sscanf(input, "%x %x", &location, &val) != 2) {
        printf("Error: Invalid input format.\n");
        return;
    }

    // Print debug logs if debug mode is active
    if (debug_mode) {
        fprintf(stderr, "Debug: location=0x%x, val=0x%x\n", location, val);
    }

    // Boundary Check: Ensure the modification stays safely within mem_buf limits
    if (location + unit_size > sizeof(mem_buf)) {
        printf("Error: Location 0x%x with unit size %d exceeds buffer boundary (Max size: %zu).\n", 
               location, unit_size, sizeof(mem_buf));
        return;
    }

    // Select the target address pointer inside mem_buf
    unsigned char* target_ptr = &mem_buf[location];

    // Overwrite the specific unit size safely using appropriate casting
    if (unit_size == 1) {
        *target_ptr = (unsigned char)val;
    } else if (unit_size == 2) {
        *((unsigned short*)target_ptr) = (unsigned short)val;
    } else if (unit_size == 4) {
        *((unsigned int*)target_ptr) = (unsigned int)val;
    }

    printf("Successfully modified unit at memory location 0x%x\n", location);
}