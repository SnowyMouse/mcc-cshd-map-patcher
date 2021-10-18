/* 
 * MCC CEA Client-Side Hit Detection Map Patcher
 *
 * Copyright (c) 2021 Snowy Mouse
 *
 * This program is licensed under version 3 of the GNU General Public License as
 * published by the Free Software Foundation in 2007. See COPYING for more info.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

static bool is_ignored(const char *path) {
    return strcmp(path, "weapons\\sniper rifle\\sniper rifle") == 0;
}

int main(int argc, const char **argv) {
    // Oops
    if(argc != 3 || (strcmp(argv[2], "undo") != 0 && strcmp(argv[2], "patch") != 0)) {
        printf("Usage: %s <map> <patch/undo>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Get the operation
    bool undo = strcmp(argv[2], "undo") == 0;
    
    // Read the map
    FILE *f = fopen(argv[1], "rb");
    if(!f) {
        fprintf(stderr, "Failed to open %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    fseek(f, 0, SEEK_END);
    size_t s = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(s);
    
    // Did data allocate?
    if(data == NULL) {
        fprintf(stderr, "Failed to allocate %zu bytes\n", s);
        free(f);
        return EXIT_FAILURE;
    }
    fread(data, s, 1, f);
    fclose(f);
    f = NULL;
    
    // We're using goto, so these variables need defined early
    int success = EXIT_FAILURE;
    uint8_t *tag_data;
    uint32_t tag_data_length;
    
    // Do a basic check
    if(*(uint32_t *)(data) != 'head' || *(uint32_t *)(data + 0x7FC) != 'foot') {
        fprintf(stderr, "%s has an invalid cache file header\n", argv[1]);
        goto cleanup;
    }
    
    // Is it an MCC CEA map?
    if(*(uint32_t *)(data + 0x4) != 13) {
        fprintf(stderr, "%s is not a MCC CEA cache file.\nThis will not work on non-MCC CEA maps.\n", argv[1]);
        goto cleanup;
    }
    
    // Is it a multiplayer map?
    if(*(uint32_t *)(data + 0x60) != 1) {
        fprintf(stderr, "%s is not a multiplayer map\nThis should only be used on multiplayer maps.\n", argv[1]);
        goto cleanup;
    }
    
    // Is the tag data length wrong?
    tag_data = data + *(uint32_t *)(data + 0x10);
    tag_data_length = *(uint32_t *)(data + 0x14);
    if(tag_data_length < 0x28) {
        fprintf(stderr, "%s does not have enough tag data to be a valid cache file.\n", argv[1]);
        goto cleanup;
    }
    
    // We have a "go" for editing the map now
    {
        uint32_t base_memory_address = *(uint32_t *)(tag_data) - 0x28;
        uint32_t max_pointer = base_memory_address + tag_data_length;
        uint32_t pistol_address = 0;
        
        #define CHECK_PTR_CHECK(ptr, len) !((ptr) < base_memory_address || (ptr) > max_pointer || ((uint64_t)(ptr) + (uint64_t)(len)) >= max_pointer)
        #define CHECK_PTR_ASSERT(ptr, len) if(!CHECK_PTR_CHECK(ptr, len)) { \
            fprintf(stderr, "Failed to translate %08X to an offset - out of bounds\n", (ptr)); \
            goto cleanup; \
        }
        #define TRANSLATE_PTR(ptr) (tag_data + (ptr) - base_memory_address)
        #define TAG_SIZE 0x20
        #define PISTOL_PATH "weapons\\pistol\\pistol"
        
        // Get the tag count
        uint32_t tag_count = *(uint32_t *)(tag_data + 0xC);
        CHECK_PTR_ASSERT(*(uint32_t *)(tag_data), tag_count * TAG_SIZE);
        uint8_t *tag_array = TRANSLATE_PTR(*(uint32_t *)(tag_data));
        
        // Go through each tag really quickly
        for(uint32_t t = 0; t < tag_count; t++) {
            uint8_t *tag = tag_array + t * TAG_SIZE;
            if(*(uint32_t *)tag != 'weap') {
                continue;
            }
            uint32_t tag_path_pointer = *(uint32_t *)(tag + 0x10);
            uint32_t tag_path_pointer_test = tag_path_pointer;
            const char *tag_path;
            size_t tag_path_length;
            
            CHECK_PTR_ASSERT(tag_path_pointer, 1);
            tag_path = (const char *)TRANSLATE_PTR(tag_path_pointer);
            
            // Make sure the paths are valid
            while(1) {
                tag_path_pointer_test++;
                CHECK_PTR_ASSERT(tag_path_pointer_test, 1);
                
                const char *t_test = (const char *)TRANSLATE_PTR(tag_path_pointer_test);
                if(*t_test == 0) {
                    break;
                }
            }
            
            // Ignore these
            if(is_ignored(tag_path)) {
                continue;
            }
            
            // Check if we have something here
            if(undo) {
                uint32_t original_pointer_maybe = *(uint32_t *)(tag + 0x1C);
                
                // Check if it's the pistol and we have 
                if(strcmp(tag_path, PISTOL_PATH) == 0 && CHECK_PTR_CHECK(original_pointer_maybe, 1)) {
                    uint32_t original_pointer_maybe_possibly = original_pointer_maybe;
                    while(1) {
                        original_pointer_maybe_possibly++;
                        CHECK_PTR_ASSERT(original_pointer_maybe_possibly, 1);
                        const char *t_test = (const char *)TRANSLATE_PTR(original_pointer_maybe_possibly);
                        if(*t_test == 0) {
                            break;
                        }
                    }
                    
                    *(uint32_t *)(tag + 0x10) = *(uint32_t *)(tag + 0x1C);
                    *(uint32_t *)(tag + 0x1C) = 0;
                }
                else {
                    fprintf(stderr, "%s is not patched. Use 'patch' to patch.\n", argv[1]);
                    goto cleanup;
                }
            }
            
            // Check if this is the pistol
            else if(!undo) {
                if(strcmp(tag_path, PISTOL_PATH) == 0) {
                    if(pistol_address == 0) {
                        pistol_address = tag_path_pointer;
                    }
                    else {
                        fprintf(stderr, "%s is already patched. Use 'undo' to undo.\n", argv[1]);
                        goto cleanup;
                    }
                }
                else if(*(uint32_t *)(tag + 0x1C) != 0) {
                    fprintf(stderr, "%s has been modified by a tool that wasn't this one.\n", argv[1]);
                    goto cleanup;
                }
            }
        }
        
        // Destruction rains from the heavens
        if(!undo) {
            // Lavos destroyed the world. Oops.
            if(pistol_address == 0) {
                fprintf(stderr, "No tag " PISTOL_PATH ".weapon was found in %s\n", argv[1]);
                goto cleanup;
            }
            
            // Go through each tag and move the pistol path here
            for(uint32_t t = 0; t < tag_count; t++) {
                uint8_t *tag = tag_array + t * TAG_SIZE;
                if(*(uint32_t *)tag != 'weap') {
                    continue;
                }
                const char *tag_path = TRANSLATE_PTR(*(uint32_t *)(tag + 0x10));
                
                // If it's ignored, drop it
                if(is_ignored(tag_path)) {
                    continue;
                }
                
                // Back up the original pistol path
                *(uint32_t *)(tag + 0x1C) = *(uint32_t *)(tag + 0x10);
                *(uint32_t *)(tag + 0x10) = pistol_address;
            }
        }
    }
    
    // Write the result
    f = fopen(argv[1], "wb");
    if(!f) {
        fprintf(stderr, "Failed to open %s for writing.\n", argv[1]);
        goto cleanup;
    }
    if(!fwrite(data, s, 1, f)) {
        fprintf(stderr, "Failed to write to %s.\n", argv[1]);
        fclose(f);
        goto cleanup;
    }
    fclose(f);
    success = EXIT_SUCCESS;
    
    // Done
    if(undo) {
        printf("%s was successfully unpatched\n", argv[1]);
    }
    else {
        printf("%s was successfully patched\n", argv[1]);
    }
    
    // Yes I use goto in my C code. Don't judge.
    cleanup:
    free(data);

    // Return the result (typically 1 on failure, 0 on success)
    return success;
}
