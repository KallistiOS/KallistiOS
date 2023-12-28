This document has been partially cribbed from Linus Torvald's excellent
and witty CodingStyle document for the Linux kernel. I don't always
agree with his comments, so where his ideas match up with mine, I have
simply included the section verbatim. Where there are only minor
differences, I've put a small note in []'s.

The sections at the bottom are entirely written by me.

These guidelines apply to all of the core KOS pieces (including everything
in the 'kernel' and 'libc' trees, except for imported BSD parts, which
generally follow these rules pretty closely anyway). And they are guidelines,
not strict rules that I'll lop off someone's head for not following. Just
please do try to follow them if you want to commit changes to the core
KOS pieces and it will make my life easier =)

							-- Megan


1) No GNU =) [imported from Linus]

First off, I'd suggest printing out a copy of the GNU coding standards,
and NOT read it.  Burn them, it's a great symbolic gesture. 


2) Indentation [imported from Linus]

Tabs are 8 characters, and thus indentations are also 8 characters. 
There are heretic movements that try to make indentations 4 (or even 2!)
characters deep, and that is akin to trying to define the value of PI to
be 3. 

Rationale: The whole idea behind indentation is to clearly define where
a block of control starts and ends.  Especially when you've been looking
at your screen for 20 straight hours, you'll find it a lot easier to see
how the indentation works if you have large indentations. 

Now, some people will claim that having 8-character indentations makes
the code move too far to the right, and makes it hard to read on a
80-character terminal screen.  The answer to that is that if you need
more than 3 levels of indentation, you're screwed anyway, and should fix
your program. 

In short, 8-char indents make things easier to read, and have the added
benefit of warning you when you're nesting your functions too deep. 
Heed that warning. 


3) Placing Braces [imported from Linus, with a few changes]

The other issue that always comes up in C styling is the placement of
braces.  Unlike the indent size, there are few technical reasons to
choose one placement strategy over the other, but the preferred way, as
shown to us by the prophets Kernighan and Ritchie, is to put the opening
brace last on the line, and put the closing brace first, thusly:

	if (x is true) {
		we do y
	}

[KOS---

Rant about the rightness of K&R's "function braces go on a new line" 
removed. 

The standard KOS style is to put all braces on the same line, regardless
of whether it's a function or some structure inside of a function. Whatever
the prophets tell us, this point I disagree about =).

---KOS]

Note that the closing brace is empty on a line of its own, _except_ in
the cases where it is followed by a continuation of the same statement,
ie a "while" in a do-statement or an "else" in an if-statement, like
this:

	do {
		body of do-loop
	} while (condition);

and

	if (x == y) {
		..
	} else if (x > y) {
		...
	} else {
		....
	}
			
Rationale: K&R. 

Also, note that this brace-placement also minimizes the number of empty
(or almost empty) lines, without any loss of readability.  Thus, as the
supply of new-lines on your screen is not a renewable resource (think
25-line terminal screens here), you have more empty lines to put
comments on. 


4) Naming [imported from Linus, with a few changes]

C is a Spartan language, and so should your naming be.  Unlike Modula-2
and Pascal programmers, C programmers do not use cute names like
ThisVariableIsATemporaryCounter.  A C programmer would call that
variable "tmp", which is much easier to write, and not the least more
difficult to understand. 

HOWEVER, while mixed-case names are frowned upon, descriptive names for
global variables are a must.  To call a global function "foo" is a
shooting offense. 

GLOBAL variables (to be used only if you _really_ need them) need to
have descriptive names, as do global functions.  If you have a function
that counts the number of active users, you should call that
"count_active_users()" or similar, you should _not_ call it "cntusr()". 

[KOS---

As a general rule, global things should be prepended by some sort of
meaningful, short system identifier. For example, everything related
to the PVR chip is prefixed with "pvr_". A similar tactic is taken with
preprocessor macros related to the PVR, and they are prefixed with
"PVR_".

---KOS]

Encoding the type of a function into the name (so-called Hungarian
notation) is brain damaged - the compiler knows the types anyway and can
check those, and it only confuses the programmer.  No wonder MicroSoft
makes buggy programs. 

LOCAL variable names should be short, and to the point.  If you have
some random integer loop counter, it should probably be called "i". 
Calling it "loop_counter" is non-productive, if there is no chance of it
being mis-understood.  Similarly, "tmp" can be just about any type of
variable that is used to hold a temporary value. 

If you are afraid to mix up your local variable names, you have another
problem, which is called the function-growth-hormone-imbalance syndrome. 
See next chapter. 

		
5) Functions [imported from Linus]

Functions should be short and sweet, and do just one thing.  They should
fit on one or two screenfuls of text (the ISO/ANSI screen size is 80x24,
as we all know), and do one thing and do that well. 

The maximum length of a function is inversely proportional to the
complexity and indentation level of that function.  So, if you have a
conceptually simple function that is just one long (but simple)
case-statement, where you have to do lots of small things for a lot of
different cases, it's OK to have a longer function. 

However, if you have a complex function, and you suspect that a
less-than-gifted first-year high-school student might not even
understand what the function is all about, you should adhere to the
maximum limits all the more closely.  Use helper functions with
descriptive names (you can ask the compiler to in-line them if you think
it's performance-critical, and it will probably do a better job of it
that you would have done). 

Another measure of the function is the number of local variables.  They
shouldn't exceed 5-10, or you're doing something wrong.  Re-think the
function, and split it into smaller pieces.  A human brain can
generally easily keep track of about 7 different things, anything more
and it gets confused.  You know you're brilliant, but maybe you'd like
to understand what you did 2 weeks from now. 


6) Commenting [imported from Linus, with a few changes]

Comments are good, but there is also a danger of over-commenting.  NEVER
try to explain HOW your code works in a comment: it's much better to
write the code so that the _working_ is obvious, and it's a waste of
time to explain badly written code. 

Generally, you want your comments to tell WHAT your code does, not HOW. 

[KOS---

Rant about comments inside functions removed. I disagree. Linus said
that comments inside functions are a bad thing and that if you need them
then your functions have growth-hormone-imbalance syndrome. This isn't
necessarily true, and I feel like comments should be used as appropriate
given the guideline above. For example:

/* This code tries to query the CD drive; if it fails, then the callback
   is called to notify the user. */
if (cd_do_some_query() < 0)
	call_callback();

That sort of thing is OK.. this generally is not:

// Open the file
f = fopen("blagh", "r");

// Write the data
fwrite(...)

// Close the file
fclose(f);

---KOS]


7) You've made a mess of it [imported from Linus, adapt as you see fit]

That's OK, we all do.  You've probably been told by your long-time Unix
user helper that "GNU emacs" automatically formats the C sources for
you, and you've noticed that yes, it does do that, but the defaults it
uses are less than desirable (in fact, they are worse than random
typing - a infinite number of monkeys typing into GNU emacs would never
make a good program). 

So, you can either get rid of GNU emacs, or change it to use saner
values.  To do the latter, you can stick the following in your .emacs file:

(defun linux-c-mode ()
  "C mode with adjusted defaults for use with the Linux kernel."
  (interactive)
  (c-mode)
  (c-set-style "K&R")
  (setq c-basic-offset 8))

This will define the M-x linux-c-mode command.  When hacking on a
module, if you put the string -*- linux-c -*- somewhere on the first
two lines, this mode will be automatically invoked. Also, you may want
to add

(setq auto-mode-alist (cons '("/usr/src/linux.*/.*\\.[ch]$" . linux-c-mode)
                       auto-mode-alist))

to your .emacs file if you want to have linux-c-mode switched on
automagically when you edit source files under /usr/src/linux.

But even if you fail in getting emacs to do sane formatting, not
everything is lost: use "indent".

Now, again, GNU indent has the same brain dead settings that GNU emacs
has, which is why you need to give it a few command line options. 
However, that's not too bad, because even the makers of GNU indent
recognize the authority of K&R (the GNU people aren't evil, they are
just severely misguided in this matter), so you just give indent the
options "-kr -i8" (stands for "K&R, 8 character indents"). 

"indent" has a lot of options, and especially when it comes to comment
re-formatting you may want to take a look at the manual page.  But
remember: "indent" is not a fix for bad programming. 


8) Header files

Header files should generally start off with the standard KOS header text,
which goes something like this:

/* KallistiOS ##version##

   foobar.h
   (c)2002 Joe Sixpack Developer

*/


This is then followed by a header duplication guard, which should generally
match the standard include path of the header when it is included. For
example, if the header is usually included with:

#include <ps2/usb.h>

Then the header guard should look like this:

#ifndef __PS2_USB_H
#define __PS2_USB_H

And at the bottom of the header, you'll want the matching #endif:

#endif	/* __PS2_USB_H */

Note that as a rule, these #ifdef/#endif pairs should be marked in this
way when the two parts are sufficiently far apart (or they will be in 
the future, potentially) that you may not be able to see both on the same
screen. Unlike brace blocks, people generally do not indent between them
so it's hard to follow.

After the header guard, you should put a short comment describing the 
purpose of the header, such as:

/*

  Declarations for the PS2's USB controller

*/

You should then put the standard C++ guard:

#include <sys/cdefs.h>
__BEGIN_DECLS

This will automatically put the 'extern "C" {' if it's needed.

Each function / struct / etc should have a comment describing its usage
before it, and then the function prototype itself. If it's too long to
fit in a reasonable amount of columns, then break it at a parameter
and tab in once on the next line:

/* Send a command to an attached peripheral; return <0 on failure. */
int ps2usb_send_command(int unit, const uint8 * data, int data_len,
	int more, int cool, int parameters);

Note however, that "more", "cool", and "parameters" are a bad idea for
parameter names ;-)

After the declarations, put the end C++ guard:

__END_DECLS

And then the above-mentioned header inclusion guard.


9) Source files

Source code files are similar to header files. They should look something
like this at the top:

/* KallistiOS ##version##

   foobar.c
   (c)2002 Joe Sixpack Developer
*/


Please minimize the number of included headers whenever possible -- take
one look at the Linux or BSD kernels to see why this is a good idea.
Generally, the ANSI headers will go first, followed by KOS headers, followed
by arch-specific headers:

#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <kos/net.h>
#include <dc/net/broadband_adapter.h>
#include <arch/types.h>

After that, feel free to put a block comment explaining what's in the file
and describing any notes about the module, if necessary / warranted,
followed by the functions/variables themselves.

If the file is so large that it needs multiple sections, then you might
want to consider splitting it up. However, if this is not possible, then
put some section separators between them like this:

/********************************************************************/
/* Variables */

static int blagh;

/********************************************************************/
/* Main functions */

...

Please also stick to static variables whenever possible. It makes it
easier on you because you know that some code in the future won't stomp
on your values by accidentally using the wrong variable, and it makes it
easier for future developers by not clouding up the name space.


10) Type names

Type names, like global function names, should be used where it will
increase clarity, and should potentially be prepended by a subsystem
identifier. Use to your own discretion, especially where you see the
potential for utility in the future for changing what the type is. For
example:

/* useless */
typedef int int_t;

/* maybe useful */
typedef uint32 net_token_t;

It's always easier to decrease the amount of specificity later on if you
change your mind, using search and replace, than it is to add it.

Note also that, following the normal ANSI/*nix guidelines, typedef'd
names should generally have a _t suffix, while non-typedef'd types should
not. You should also never create an anonymously typedef'd structure.
Example:

/* Bad */
typedef struct {
} blagh;

/* Good */
typedef struct blagh {
} blagh_t;



