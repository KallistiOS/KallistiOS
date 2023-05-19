#include <stdio.h>
#include <objc/objc.h>
#import <objc/Object.h>
#include <objc/runtime.h>

@interface Person: Object 
 {
    int douchewezel;
 }
- (void)test;
+ (void)initialize;
@end

@implementation Person
+ (void) initialize {
    printf("Objective-C static constructors work!\n\n");
}

 - (void)test {
    printf("ObjC method dispatch with class reflection work %s\n", class_getName([self class]));
    fflush(stdout);
}  
@end

int main(int argc, char *argv[]) {
    printf("Objective C Class Location!: %p", [Person class]);
    Person* person = class_createInstance(objc_getClass("Person"), 0);
    printf("Objective C Instance Location = %p", person);
    [person test];

    printf("C API works %s\n", class_getName(object_getClass(person)));

    printf("ObjC method again: %d\n", [person isEqual:person]);
    fflush(stdout);
    return 0;
}