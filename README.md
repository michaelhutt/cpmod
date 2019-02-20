# cpmod
copy the file mode bits from one place to another

Usage:
>  cpmod [-r] spec FILE [FILE...]
  
Where 'spec' is a two letter code specifying a 'from' position and a 'to' position. Either letter can be one of 'u', 'g', or 'o' for user, group, and other respectively.

Example:
>  cpmod -r ug ./some_dir

...will copy the permissions from the 'user' to the 'group' for the directory ./some_dir and everything under it.
