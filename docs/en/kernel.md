# An overview of the kernel
The Linux kernel is a very large-scale project, developers from all over the world are working on it. Hundreds of emails, patches and commits are being sent every day. Only few pieces of code 
manages to escape this mess actually makes it way into the kernel.
Even though this code examined. Security guys find at least one or two different kernel critical 
vulnerabilities every year.

However, to understand these vulnerabilities, we must first understand the kernel.

### Kernel-space and User-space
Your email client, your browser, your text editor, all these programs run in user-space.
These programs can only use the resources that the operating system/kernel let them use.
They cannot access all the resources, for example a program in user-space, cannot read another 
program's memory unless the kernel allows it.

On the other hand, the kernel of the operating system and the drivers that work with the kernel 
are the most authoritative programs on the system and they run in the kernel-space, they access hardware 
resources directly, and they manage them correctly so that other programs can use them, 
they do not have any restrictions. They can directly read any memory.

It is possible to switch from kernel-space to user-space. In other words, if the kernel wants, 
it can access user-space program memories, kill these programs etc. But its not possible to 
switch from user-space to the kernel-space. A program running in user-space can never ever access 
kernel memory neither it can kill the kernel.

But the kernel vulnerabilities allows us to do exactly that. They let us access to the kernel-space 
from the user-space. Once you access the kernel-space, the rest is up to you. If you want, you can 
become the root, you can crash the kernel, the whole system is in your hands.

Now that we understand the kernel-space and user-space stuff in general, how do all 
these vulnerabilities in the kernel work?

### Different types of kernel vulnerabilities
There are many different kernel vulnerabilities, and although they are all different, the underlying
concepts are the same. Modern kernel vulnerabilities can be grouped into the following classes:

- **Integer Underflow/Overflow**: This occurs when a integer variable overflows or underflows. Depending on the case, this variable may be rounded to some other number, making different memory based attacks 
possible.

- **Arbitrary Memory Read**: This occurs when reading operations from memory are not controlled or 
limited correctly. This type of vulnerability allows us to read arbitrary program memory, which can 
reveal sensitive information and make different memory based attacks possible.

- **Arbitrary Memory Write**: This occurs when the write operations to the memory are not controlled or 
limited correctly. This type of vulnerability allows us to write arbitrary program memory, this 
may let us change the flow of the program.

- **Use-After-Free (UAF)**: This occurs when a program accesses a memory after its freed. It opens the door to different memory attacks.

- **Buffer Overflow**: Occurs when a buffer in the memory overflows. In this case, the memory below
this buffer can be modified, thus the flow of the program can be changed.

- **Race-Condition**: Occurs when two or more objects try to access the same resource at the same time.
This results a "race" for the resource. Unlike the other vulnerability types, this one is not 
based on memory. However it may cause one the above memory-based vulnerabilities.

**IMPORTANT:** These vulnerability classes are not specific to kernel, you can find 
these in the user-space programs as well.

As you may have noticed, most types of vulnerabilities are based on memory. This is why 
the kernel protections that we will take a look right now are generally based on memory.

### Kernel korumaları
Kernel uses different protection methods/technologies to protect itself from possible vulnerabilities.
It is very important for us to know these so that we can later learn to bypass them:

- **KASLR (Kernel address space layout randomization)**: Under normal circumstances, when kernel is 
loaded to the memory, it loads to the same exact address. For this reason functions and similar symbols 
are also loaded to the same address. For example if `prepare_kernel_cred` is loaded 
to `0xffffffff81094a50`, this address will not change when the system is restarted.  An attacker
who knows addresses for any critical functions can redirect the flow of the kernel to these functions
by using a vulnerability that allows this. In order to prevent this KASLR randomizes where the kernel
loads, therefore it also randomizes addresses of the kernel functions and the symbols. So `prepare_kernel_cred` which is at address `0xffffffff81094a50` maybe located at `0xffffffff421631e8`
after a reboot. This prevents attacker from redirecting the control flow, since now they need to 
find the new address. KASLR can be disabled by adding `nokaslr` to kernel parameters.

- **Stack Cookies**: Similar to stack cookies in user-space, kernel-space places a "cookie" on the stack. 
If this cookie is modified in any way during the execution of the kernel, kernel stops the execution 
and panics (kernel panic, also known as BSOD). This prevents classic **buffer overflow** exploits.

- **SMEP (Supervisor Mode Execution Prevention)**: SMEP, which is a feature of some CPU implementations
ensures that the user-space memory is "non-executable". If an attacker compromises the kernel 
through a vulnerability and redirects control flow to the user-space, SMEP causes a "trap" at the 
CPU level, which then gets handled by the kernel. **This does not cause a kernel panic, kernel just
stops the execution of the current task and goes for the next, but its logged to the kernel logs 
(dmesg)**. SMEP can be turned off by adding `nosmep` to the kernel parameters.

- **SMAP (Supervisor Mode Execution Prevention)**: SMAP, which is also a feature of CPU implementations,
ensures that the user-space memory is not "accessable". If kernel wants to access the user-space memory,
it uses `STAC` and `CLAC` to make the user-space memory accessable/inaccessible. If an attacker 
compromises the kernel using a vulnerability, and tries to copy memory from the user-space, SMAP 
causes a "trap" at CPU level, which gets handled by the kernel exactly the same way.
SMAP can be turned off by adding `nosmap` to the kernel parameters.

- **KAISER (Kernel page-table isolation)**: KAISER prevents side-channel attacks such as meltdown.
This protection basicaly creates a copy of the user-space memory for the kernel. When the CPU 
switchs to kernel mode, kernel copies the user-space memory to the kernel-space. So KAISER completely separates the user-space and the kernel-space memory, giving attacker no way to access user-space 
from the kernel-space. KAISER can be turned off by adding `nopti` to the kernel parameters.

Although kernel protections seem quite advanced and scary, there are different methods to bypass them.
But you don't have to worry about these, these protections will disabled in our lab. Except for the 
stack cookies, but we'll easily bypass it.

### Kernel panic
If the kernel gets into a weird situation and if it cannot safely recover, 
**B**lack **S**screen **O**f **D**eath (BSOD) occurs. This screen (which is special to Unix systems)
actually gives us a lot of information. This information includes stack status, 
basic registers and their values, and many other similar information.

### Kernel modules
Kernel modules are binaries that can be added/removed from the kernel at the runtime.
Kernel modules are generally used as drivers, these modules are loaded to the kernel-space 
and they can access to kernel functions.

If you are wondering why I am talking about kernel modules, the vulnerability we will exploit 
is in a module. Adding a vulnerability by directly changing the source of the kernel requires much more 
effort than writing a simple vulnerable kernel module, its also much more difficult to distribute.
Since you will be working with a vulnerable module in this challenge/guide and in other CTF challenges
it is useful to know what these modules are.

We will talk about the kernel modules in more detail while examining our vulnerable module 
in the following sections.

---
[Previous](../../README.md) | [Next](setup.md)
