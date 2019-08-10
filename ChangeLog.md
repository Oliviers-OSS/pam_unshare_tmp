## v1.0.3.0 (2019-08-10)
* IPC name space can now be unshared between users (new key name unshare_ipc, boolean value in the configuration file)

## v1.0.2.2 (2019-08-05) [View](https://github.com/Oliviers-OSS/pam_unshare_tmp/commit/792e9ecabeb9cc6c1a087cb685f86638cba90fd2)
* Configuration file allows to set users'configuration parameters using their primary group

## v1.0.2.1 (2019-08-05) [View](https://github.com/Oliviers-OSS/pam_unshare_tmp/commit/63df7eeb56a4231d1c1671924a9ba943034717c8) 
* Default parameters can now be set in the configuration file
* BUGFIX: dirmode parameter was not set and was overwriting the mountflags
* BUGFIX: harcoded mount flags debug value removed  

## v1.0.2.0 (2019-07-31) [View](https://github.com/Oliviers-OSS/pam_unshare_tmp/commit/beecd800142b88fb0e8b492e0a06fcea6d21597e)
*  Use utmpx API to unmount the user's tmp directory

## v1.0.1.0 (2019-07-31) [View](https://github.com/Oliviers-OSS/pam_unshare_tmp/commit/860aaf69dee3ce8e8d26882f955a6fd841b84f56)
*  /var/tmp management added 
*  script to update ChangeLog added (based on https://stackoverflow.com/questions/40865597/generate-changelog-from-commit-and-tag )
 
## v1.0.0.0 (2019-07-30) [View](https://github.com/Oliviers-OSS/pam_unshare_tmp/commit/47c429df1ead65d0abedcf7bc581fe6342734be9)
*  First version 
