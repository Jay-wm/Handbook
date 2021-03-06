# SVN

## 一、Subversion----软件简介
  SVN属于集中式的版本控制系统
  集中式的版本控制系统都有一个单一的集中管理的服务器，保存所有文件的修订版本，而协同工作的人们都通过客户端连到这台服务器，取出最新的文件或者提交更新。

特点：
  -- 每个版本库有唯一的URL（官方地址），每个用户都从这个地址获取代码和数据；
  -- 获取代码的更新，也只能连接到这个唯一的版本库，同步以取得最新数据；
  -- 提交必须有网络连接（非本地版本库）；
  -- 提交需要授权，如果没有写权限，提交会失败；
  -- 提交并非每次都能够成功。如果有其他人先于你提交，会提示“改动基于过时的版本，先更新再提交”… 诸如此类；
  -- 冲突解决是一个提交速度的竞赛：手快者，先提交，平安无事；手慢者，后提交，可能遇到麻烦的冲突解决。

优点：每个人都可以一定程度上看到项目中的其他人正在做些什么。而管理员也可以轻松掌控每个开发者的权限。

缺点：中央服务器的单点故障。
  若是宕机一小时，那么在这一小时内，谁都无法提交更新、还原、对比等，也就无法协同工作。如果中央服务器的磁盘发生故障，并且没做过备份或者备份得不够及时的话，还会有丢失数据的风险。
  最坏的情况是彻底丢失整个项目的所有历史更改记录，被客户端提取出来的某些快照数据除外，但这样的话依然是个问题，你不能保证所有的数据都已经有人提取出来。

Subversion原理上只关心文件内容的具体差异。每次记录有哪些文件作了更新，以及都更新了哪些行的什么内容。

服务器端（Visual SVN）
  --不分版本号：32/64都可

客户端
  --分32/64位操作系统
  --安装完成要重启电脑

## 二、使用步骤

  一个项目的开始

1. 搭建SVN服务器（192.168.1.1）
2. 项目经理在服务器上搭建项目(project)、配置相关文件（服务器端）
3. 项目经理建立本地数据（客户端）
       `checkout`：检出，第一次由服务器检出到本地
       `update`：更新，将服务器中更改的文件更新到本地
       `commit`：提交，把本地更新提交到服务器 ---- 初始化文件：core.cpp、核心函数库：common
4.   新程序员
     得到SVN服务器地址：<svn://192.168.1.1/project>
     `checkout`：检出、由服务器到客户端
     `update`：更新、由服务器到客户端
     `commit`：提交、由客户端到服务器



## 三、使用详解

1. 图表集
       --绿色对勾：客户端与服务器端完全同步
       --感叹号：客户端提交的文件与服务器数据冲突
       --X已删除：服务器相关数据已删除
       --+增加：当客户端编写的文件已添加到提交队列
       --？无版本控制:当客户端编写的文件没有添加到提交队列
       --！修改：客户端文件修改未提交
       --只读文件
       --锁定：当服务器端数据已锁定，客户端显示
       --忽略：客户端数据忽略
2. 忽略功能
       --选中文件/文件夹  ---- > SVN ----> 增加文件/文件夹/类型文件到忽略
3. 监管 `svnserve -d -r`
       `cmd` 窗口运行 `svnserve -d -r folder`
         -d：后台运行
         -r：监管目录
4. <svn://localhost> 或 `ip` 地址访问 <D:/svn/projects> 目录
       如果需要访问 projects 里面的某一个项目 <svn://localhost/projectname>

5. 权限控制

   1. 开启权限功能：每一个仓库中都有一个 `conf `文件夹，里面有三个文件

      `authz`：授权文件，默认禁用
      `passwd`：认证文件、默认禁用
      `svnserve.conf`：配置文件
      
   2. 开启步骤：
    --打开 `svnserve.conf`
     --注释匿名用户的可读写权限：`# anon-access = write`
     --开启授权和认证文件：取消注释 `authz-db = zuthz、password-db = passwd`

   3. --编写认证文件定义相关用户名和密码：`passwd`
      `username1 = password1`
      `username2 = password2`
      
   4. 编写授权文件：`authz`

      1. ` [groups]`：分组操作
           `groupname1 = username1, username2`
      2. `[projectname:/]`
            `@gruopname1 = rw `       --r：可读    --w：可写
      3. `*= r  `                 ----匿名用户，可读权限

   5. 完成：提交文件时需要登录用户



## 四、常见问题

1. 版本冲突：提前有人提交同一文件修改数据
       --分开时间开发
       --分配不同模块
       --通过 SVN

2. 更新服务器数据到本地

3. `filename` ---- 整合后的文件

   `filename.mine` ---- 本人修改的文件

   `filename.version` ---- 更新前的，别人先更新后的

4. 删除.version、mine文件，修改整合后文件再提交
       

