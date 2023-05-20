/* KallistiOS ##version##

   runtime.m
   Copyright (C) 2023 Falco Girgis, Andrew Apperley
*/

#import <objc/objc.h>
#import <objc/Object.h>
#import <objc/runtime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct objc_ivar
{
	/**
	 * Name of this instance variable.
	 */
	const char *name;
	/**
	 * Type encoding for this instance variable.
	 */
	const char *type;
};

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
@property(nonatomic)Person *bestFriend;
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

void printIVarsForPerson(Person *person) {
    unsigned int outCount;
    int i;
        Ivar *iVarList = class_copyIvarList(objc_getClass("Person"), &outCount);
    for(i = 0; i < outCount; i++) {
        Ivar _iVar = iVarList[i];
        printf("iVar Name::%s \n", _iVar->name);
        if (strcmp(_iVar->type, "i") == 0) {
            printf("iVar Value::%d \n", object_getIvar(person, _iVar));
        } else if (strcmp(_iVar->type, "r*") == 0) {
            printf("iVar Value::%s \n", object_getIvar(person, _iVar));
        } else if (strcmp(_iVar->type, "f") == 0) {
            printf("iVar Value::%f \n", object_getIvar(person, _iVar));
        } else if (strcmp(_iVar->type, "C") == 0) {
            printf("iVar Value::%s \n", object_getIvar(person, _iVar) == 1 ? "true" : "false");
        } else if (strcmp(_iVar->type, "@\"Person\"") == 0) {
            printf("iVar Value::0x%x \n", object_getIvar(person, _iVar));
        }
    }
    free(iVarList);
}

int main(int argc, char *argv[]) {
    // Make Person instance
    Person *person1 = class_createInstance(objc_getClass("Person"), 0);
    [person1 addName: "Joe" age: 20 height: 6.1];
    // Print Persons Name
    Ivar ivar = class_getInstanceVariable(objc_getClass("Person"), "_name");
    const char *name = object_getIvar(person1, ivar);
    printf("Person Name: %s \n", name);
    // Print ivars
    printf("Person iVars Start:: \n");
    printIVarsForPerson(person1);
    printf("Person iVars End:: \n");
    // Check if instance of Person responds to getBestFriend
    BOOL respondsToGetBestFriendMethod = class_respondsToSelector(objc_getClass("Person"), @selector(bestFriend));
    printf("Person responds to getBestFriend::%s \n", respondsToGetBestFriendMethod == 1 ? "true" : "false");
    // Make instance of Person for bestfriend
    Person *person2 = class_createInstance(objc_getClass("Person"), 0);
    [person2 addName: "Jim" age: 30 height: 5.2];
    [person2 setDead: YES];
    // Add bestfriend to first person
    object_setInstanceVariable(person1, "bestFriend", person2);
    // Print ivars
    printf("Person iVars Start:: \n");
    printIVarsForPerson(person2);
    printf("Person iVars End:: \n");
    fflush(stdout);

    if (!person1.bestFriend) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}