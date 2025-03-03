**Linux Multi-User**

The Linux operating system is inherently designed to support multiple users 
simultaneously, which is one of its core features. Below is a brief document 
explaining the Linux multi-user model.

---

**Introduction:**

Linux is a multi-user operating system, which means it allows multiple users to 
access the system resources concurrently. Each user has their own environment 
and can run processes independently of others. This model is essential for 
servers, enterprises, and environments where resources need to be shared 
securely and efficiently.

**Key Concepts:**

1. **User Accounts:**

   - Every user in Linux has a unique user account identified by a User ID (UID).
   - User accounts are defined in the `/etc/passwd` file.
   - Each user has a home directory, typically located in `/home/username`.

2. **Groups:**

   - Users can be part of one or more groups.
   - Groups are used to manage permissions for multiple users collectively.
   - Group information is stored in the `/etc/group` file.

3. **Permissions:**

   - Linux uses a permission system to control access to files and directories.
   - Permissions are defined for three types of users: the owner, the group, 
     and others.
   - Permissions include read (r), write (w), and execute (x).

4. **Process Isolation:**

   - Each user’s processes are isolated from others.
   - The operating system ensures that one user cannot interfere with another 
     user’s processes.

5. **Resource Management:**

   - Linux provides mechanisms like quotas and limits to manage resource usage 
     per user.
   - Tools like `ulimit` and `cgroups` can be used to set limits on resources 
     such as CPU, memory, and disk space.

**Advantages of the Multi-User Model:**

- **Resource Sharing:** Multiple users can share the same system resources 
  efficiently.
- **Security:** User isolation and permissions ensure that users cannot access 
  or modify each other’s files without proper authorization.
- **Scalability:** Suitable for environments ranging from small teams to large 
  enterprises.
- **Cost-Effective:** Reduces the need for multiple physical machines.

**Tools and Utilities**

Linux provides command-line tools for managing users, groups, and permissions.  
These tools use the underlying system calls and libraries:

#### **User Management:**
- **`useradd`**: Add a new user.
- **`usermod`**: Modify an existing user account properties.
- **`userdel`**: Delete a user.
- **`passwd`**: Set or change a user’s password.

#### **Group Management:**
- **`groups`**: Display the groups a user belongs to.
- **`groupadd`**: Add a new group.
- **`groupmod`**: Modify an existing group.
- **`groupdel`**: Delete a group.
- **`gpasswd`**: Manage group passwords and members.

#### **Permission Management:**
- **`chmod`**: Change file permissions.
- **`chown`**: Change file ownership.
- **`chgrp`**: Change file group ownership.

**Configuration Files**

Linux stores multi-user information in configuration files, which can be 
manipulated directly or through APIs:

- **`/etc/passwd`**: Contains user account information.
- **`/etc/shadow`**: Stores encrypted passwords and password-related settings.
- **`/etc/group`**: Contains group information.
- **`/etc/gshadow`**: Stores encrypted group passwords (rarely used).

**Linux Multi-User API**

The Linux multi-user model is deeply integrated into the operating system's 
design, and its functionality is exposed through a combination of system calls, 
libraries, and configuration files. Below is an elaboration on the **Linux 
Multi-User API**, which includes the key system calls, libraries, and tools 
used to manage users, groups, permissions, and resources in a multi-user 
environment.

### **1. System Calls for User and Group Management**

System calls are the low-level interfaces provided by the Linux kernel for 
managing users, groups, and permissions. These are typically used by system 
utilities and programs to interact with the kernel.

#### **User Management:**
- **`getuid()`**: Get the real user ID of the calling process.
- **`geteuid()`**: Get the effective user ID of the calling process.
- **`setuid(uid_t uid)`**: Set the user ID of the calling process.
- **`seteuid(uid_t uid)`**: Set the effective user ID of the calling process.
- **`getpwnam(const char *name)`**: Retrieve user information (from 
  `/etc/passwd`) by username.
- **`getpwuid(uid_t uid)`**: Retrieve user information by user ID.

#### **Group Management:**
- **`getgid()`**: Get the real group ID of the calling process.
- **`getegid()`**: Get the effective group ID of the calling process.
- **`setgid(gid_t gid)`**: Set the group ID of the calling process.
- **`setegid(gid_t gid)`**: Set the effective group ID of the calling process.
- **`getgrnam(const char *name)`**: Retrieve group information (from `/etc/group`) by group name.
- **`getgrgid(gid_t gid)`**: Retrieve group information by group ID.

#### **Switching Users and Groups:**
- **`initgroups(const char *user, gid_t group)`**: Initialize the supplementary 
  group list for a user.
- **`setgroups(size_t size, const gid_t *list)`**: Set the supplementary group 
  IDs for the calling process.

### **2. System Calls for File Permissions**

Linux uses file permissions to control access to files and directories in a 
multi-user environment. The following system calls are used to manage 
permissions:

- **`chmod(const char *path, mode_t mode)`**: Change the permissions of a file 
  or directory.
- **`chown(const char *path, uid_t owner, gid_t group)`**: Change the ownership 
  of a file or directory.
- **`umask(mode_t mask)`**: Set the default file creation permissions mask.

### **3. Libraries for Multi-User Management**

Higher-level libraries provide abstractions for managing users, groups, and 
permissions. These libraries are often used by system utilities and 
applications.

#### **Glibc (GNU C Library):**
- Provides functions like `getpwnam()`, `getgrnam()`, `chmod()`, and `chown()`.
- Used by most Linux programs to interact with the system.

#### **PAM (Pluggable Authentication Modules):**
- A framework for authentication and authorization.
- Provides APIs for managing user sessions, passwords, and access control.
- Used by login systems like `sshd` and `login`.

#### **Shadow Password Suite:**
- Provides functions for managing encrypted passwords stored in `/etc/shadow`.
- Includes utilities like `useradd`, `usermod`, and `passwd`.

### **4. Example: Using System Calls in a Program**

Here’s an example of a C program that uses system calls to switch users and groups:

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

int main() {
    // Get the user ID for "nobody"
    struct passwd *pwd = getpwnam("nobody");
    if (pwd == NULL) {
        perror("getpwnam");
        exit(1);
    }

    // Get the group ID for "nogroup"
    struct group *grp = getgrnam("nogroup");
    if (grp == NULL) {
        perror("getgrnam");
        exit(1);
    }

    // Switch to the "nobody" user and "nogroup" group
    if (setgid(grp->gr_gid) != 0) {
        perror("setgid");
        exit(1);
    }
    if (setuid(pwd->pw_uid) != 0) {
        perror("setuid");
        exit(1);
    }

    printf("Now running as user: %s (%d), group: %s (%d)\n",
           pwd->pw_name, pwd->pw_uid, grp->gr_name, grp->gr_gid);

    return 0;
}
```

**Advanced Multi-User APIs**

#### **Linux Disk Quotas**

Disk quotas in Linux are a mechanism to control and limit the amount of disk 
space and the number of files that individual users or groups can use on a 
filesystem. This is particularly useful in multi-user environments to prevent 
any single user from consuming excessive disk space, which could affect other 
users and the overall system performance.

### **Key Concepts of Disk Quotas**

1. **Types of Quotas:**
   - **User Quotas:** Limit disk usage for individual users.
   - **Group Quotas:** Limit disk usage for groups of users.

2. **Quota Limits:**
   - **Soft Limit:** A warning threshold. Users can exceed this limit 
     temporarily (within a grace period).
   - **Hard Limit:** The absolute maximum limit. Users cannot exceed this limit 
     under any circumstances.
   - **Grace Period:** The time allowed for a user to reduce their disk usage 
     after exceeding the soft limit.

3. **Quota Accounting:**
   - Quotas track two main metrics:
     - **Block Usage:** The amount of disk space used (measured in 1 KB blocks 
       by default).
     - **Inode Usage:** The number of files (inodes) created.

4. **Filesystem Support:**
   - Quotas are supported on filesystems like `ext3`, `ext4`, `XFS`, and 
     others.
   - Quotas must be enabled on the filesystem before they can be used.

### **Steps to Configure Disk Quotas**

#### 1. **Install Quota Tools**

   On most Linux distributions, the quota management tools are included in the 
   `quota` package. Install it if not already present:
   ```bash
   sudo apt install quota  # For Debian/Ubuntu
   sudo yum install quota  # For CentOS/RHEL
   ```

#### 2. **Enable Quotas on the Filesystem**

   - Edit the `/etc/fstab` file to enable quotas for the desired filesystem.  
     Add the `usrquota` and/or `grpquota` options to the filesystem entry:
     ```
     /dev/sda1  /home  ext4  defaults,usrquota,grpquota  0  2
     ```
   - Remount the filesystem:
     ```bash
     sudo mount -o remount /home
     ```

#### 3. **Initialize Quota System**

   - Run the `quotacheck` command to create quota files (`aquota.user` and 
     `aquota.group`) and scan the filesystem:
     ```bash
     sudo quotacheck -cug /home
     ```
   - Enable quotas:
     ```bash
     sudo quotaon /home
     ```

#### 4. **Set Quota Limits**

   - Use the `edquota` command to set quotas for a user or group:
     ```bash
     sudo edquota -u username
     ```
     This opens an editor where you can set soft and hard limits for blocks 
     (disk space) and inodes (number of files):
     ```
     Disk quotas for user username (uid 1001):
       Filesystem   blocks   soft   hard   inodes   soft   hard
       /dev/sda1    4000     5000   6000   100      150    200
     ```
   - To set a grace period, use:
     ```bash
     sudo edquota -t
     ```

#### 5. **Check Quota Usage**

   - Use the `quota` command to check a user’s disk usage and limits:
     ```bash
     sudo quota -u username
     ```
   - Use `repquota` to generate a report for all users and groups:
     ```bash
     sudo repquota /home
     ```

### **Common Quota Commands**

| Command                  | Description                                      |
|--------------------------|--------------------------------------------------|
| `quotacheck -cug /home`  | Create quota files and scan the filesystem.      |
| `quotaon /home`          | Enable quotas for the specified filesystem.     |
| `quotaoff /home`         | Disable quotas for the specified filesystem.    |
| `edquota -u username`    | Edit quota limits for a specific user.          |
| `edquota -g groupname`   | Edit quota limits for a specific group.         |
| `quota -u username`      | Display quota usage and limits for a user.      |
| `repquota /home`         | Generate a quota report for the filesystem.     |

### **Advantages of Using Quotas**

- **Prevent Disk Abuse:** Ensures no single user or group can monopolize disk space.
- **Fair Resource Allocation:** Promotes fair usage among multiple users.
- **Graceful Enforcement:** Soft limits and grace periods allow users to manage 
  their usage without immediate disruption.
- **System Stability:** Prevents the system from running out of disk space, 
  which could cause crashes or data loss.

### **Example Scenario**

Imagine a shared server with multiple users. To ensure no user exceeds their fair share of disk space:

1. Set a soft limit of 500 MB and a hard limit of 600 MB for each user.
2. Set a grace period of 7 days for users to reduce their usage after exceeding 
   the soft limit.
3. Regularly monitor quota usage using `repquota` to ensure compliance.

#### **Capabilities API:**
- Linux provides fine-grained privilege management through capabilities (e.g., 
  `CAP_NET_BIND_SERVICE` for binding to privileged ports).
- System calls like `capset()` and `capget()` are used to manage capabilities.

#### **Namespaces and cgroups:**
- Namespaces isolate users and processes (e.g., user namespaces for UID 
  mapping).
- cgroups (control groups) limit and account for resource usage (e.g., CPU, 
  memory) per user or group.

---

**Conclusion:**

The Linux multi-user is a robust and flexible subsystem that allows multiple 
users to work on the same machine securely and efficiently. Understanding how 
to manage users, groups, and permissions is crucial for system administrators 
to maintain a secure and organized environment.

The Linux multi-user API is a combination of system calls, libraries, and tools 
that enable the creation, management, and isolation of users and groups.  These 
APIs are essential for building secure and scalable multi-user systems, from 
small servers to large enterprise environments.

Disk quotas are a powerful tool for managing disk usage in multi-user Linux 
environments. By setting appropriate limits and monitoring usage, system 
administrators can ensure fair and efficient resource allocation while 
maintaining system stability.

vim:fo+=awn:

vim:fo+=awn:spell:
