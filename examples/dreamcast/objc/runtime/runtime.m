/* KallistiOS ##version##

   runtime.m
   Copyright (C) 2023 Falco Girgis, Andrew Apperley

   This example serves two purposes: to demonstrate the basic
   usage of the Objective-C language runtime C API as well as 
   to serve as a toolchain and sanity check to validate it. 
*/

#import <objc/objc.h>
#import <objc/Object.h>
#import <objc/runtime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*
Relatively simple Objective-C class flexing:
  - inheritance
  - instance variables
  - properties
  - message handlers
*/
@interface Person: Object 
{
    const char *_name;
    int _age;
    float _height;
    Person *_bestFriend;
    BOOL _dead;
}
- (void)addName:(const char *)name age:(int)age height:(float)height;
- (void)setBestFriend:(Person *)bestFriend;
@property(nonatomic, assign)Person *bestFriend;
@property(nonatomic)BOOL dead;
@end

@implementation Person

@synthesize bestFriend = _bestFriend, dead = _dead;

- (void)addName:(const char *)name age:(int)age height:(float)height {
    _name = name;
    _age = age;
    _height = height;
}

- (void)setDead:(BOOL)dead {
    if (_dead == YES) { return; }
    _dead = dead;
}

- (BOOL)dead {
    return _dead;
}

- (void)setBestFriend:(Person *)bestFriend {
    _bestFriend = bestFriend;
}

- (Person *)bestFriend {
    return _bestFriend;
}
@end

/*
   Utility function for reflecting over a Person, printing its
   instance variables.
*/
static BOOL printIVarsForPerson(const Person *person) {
    BOOL success = YES;
    unsigned int outCount = 0;

    // Retrieve a list of all instance variables
    Ivar *iVarList = class_copyIvarList(objc_getClass("Person"), &outCount);
    printf("Discovered %u instance variables:\n", outCount);

    // Ensure we found all 5 of them
    if(outCount != 5) {
        fprintf(stderr, "\tUnexpected count!\n");
        success = NO;
    }
    else { 
        // Iterate over them, printing name + property
        for(unsigned i = 0; i < outCount; i++) {
            Ivar iVar = iVarList[i];
            const char* type = ivar_getTypeEncoding(iVar);

            printf("\t[%u] %s: ", i, ivar_getName(iVar));

            // Use the encoded type to tell us how to print the queried value
            if(strcmp(type, "f") == 0) {
                ptrdiff_t offset = ivar_getOffset(iVar);
                printf("%f\n", *(float*)(((uint8_t*)person) + offset));
            } 
            else {
                id value = object_getIvar(person, iVar);

                const char* format;
                if (strcmp(type, "i") == 0) {
                    format = "%d";
                } 
                else if(strcmp(type, "r*") == 0) {
                    format = "%s";
                } 
                else if(strcmp(type, "C") == 0) {
                    format = "%u";
                }
                else if(strcmp(type, "@\"Person\"") == 0) {
                    format = "%x";
                }
                else {
                    fprintf(stderr, "Unexpected Type [%s]\n", type);
                    success = NO; 
                    break;
                }

                printf(format, value);
                printf("\n");
            }
        }
    }

    free(iVarList);
    return success;
}

int main(int argc, char *argv[]) {
    int result = EXIT_SUCCESS;
    Person *person1 = NULL;
    Person *person2 = NULL;

    // Create a new instance of person, using the runtime
    person1 = class_createInstance(objc_getClass("Person"), 0);

    // Verify the runtime succeeded
    if(!person1) {
        fprintf(stderr, "Failed to create Person instance!\n");
        result = EXIT_FAILURE;
        goto exit;
    } 
    else printf("Created Person instance.\n");

    // Call message handler for "addName," setting instance variables
    [person1 addName: "Joe" age: 20 height: 6.1f];

    // Dynamically query values of instance variables by string names
    const char* name = NULL;
    int age = 0;
    float height = 0.0f;
    object_getInstanceVariable(person1, "_name", (void**)&name);
    object_getInstanceVariable(person1, "_age", (void**)&age);
    object_getInstanceVariable(person1, "_height", (void**)&height);

    // Verify values were properly set and retrieved
    if(strcmp(name, "Joe") != 0 || age != 20 || height != 6.1f) {
        fprintf(stderr, "Failed to set and retrieve instance variables!\n");
        result = EXIT_FAILURE;
        goto exit;
    }
    else printf("Set and retrieved instance variables.\n");
    
    // Use runtime to dynamically reflect over all properties
    if(!printIVarsForPerson(person1)) {
        result = EXIT_FAILURE;
        goto exit;
    }

    // Check if instance of Person responds to getBestFriend
    if(!class_respondsToSelector(objc_getClass("Person"), @selector(bestFriend))) { 
        fprintf(stderr, "Does not respond to getBestFriend message!\n");
        result = EXIT_FAILURE;
        goto exit;
    }
    else printf("Checked for responding to message handler.\n");

    // Use runtime to copy construct another Person instance
    person2 = object_copy(person1, 0);
    
    if(!person2) {
        fprintf(stderr, "Failed to copy Person instance!\n");
        result = EXIT_FAILURE;
        goto exit;
    } 
    else printf("Copied Person instance.\n");

    // Initialize instance variables
    [person2 addName: "Jim" age: 30 height: 5.2];
    
    // Test out setting and retrieving properties
    person2.dead = YES;
    object_setInstanceVariable(person1, "_bestFriend", person2);

    if(!person2.dead || person1.bestFriend != person2) {
        fprintf(stderr, "Failed to set and retrieve properties!\n");
        result = EXIT_FAILURE;
    } else printf("Set and retrieved properties!\n");

exit:

    // Clean up after ourselves
    object_dispose(person1);
    object_dispose(person2);

    if(result == EXIT_FAILURE) 
        fprintf(stderr, "**** FAILURE! ****\n");
    else  
        printf("**** SUCCESS! ****\n");
    
    return result;
}
