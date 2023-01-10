# HMIS

Store patient records and generate a monthly HMIS 105 report for your medical facility.

![HMIS Screenshot](./hmis.png)

This software is meant to help you aggregate data for attendances and diagnoses only. 
It is not a complete hospital management system.

If you are interested in a complete hospital system, [send me an email](mailto:nabiira2by2@gmail.com).

## Features
- Register Patients
- Auto-generate HMIS 105 report for attendances and diagnoses
- Backups and restore
- View patient register with custom search by Ip Number or diagnosis.


## Installation:
1. Download the latest installer for windows from the releases page at https://github.com/abiiranathan/hmis/releases/tag/v1.0.0
   
2. Clone the repository and build you own installer.
   
See [QtInstaller Project](https://github.com/abiiranathan/qtinstaller) and learn how to create windows installers for Qt applications with ease.
---

## Database configuration
Your HMIS data is stored in your home folder with database name hmis.sqlite3. e.g 
`C:\Users\username\hmis.sqlite3`. 
You can customize this with an environment variable HMIS_DB.

> If you are updating from tag v1, your database is in the same location as your main binary.
> ** First copy your database *db.sqlite3* from `C:\ProgramFiles\HMIS\db.sqlite3` to another location before uninstalling the previous version.
> After updating, go to Backup menu and restore the data to the new database.

## Known issues

HMIS uses a sqlite3 database. When running HMIS on windows, you may get an `Error opening database`. This is because the application needs read and write permissions on the C:\ drive.

To solve this issue, right click on the Desktop Icon -> Properties -> Compatibility tab -> Check Run this program as an Administrator.

 This enables HMIS to run as Administrator persistently. The longer option is to manually change permissions from the Security Tab under Properties.


---
### Futures am thinking about but not yet implemented.
* Integrated HAART register and reports
* Antenatal Register and reports
* Support other database backends e.g postgres, mysql through environment variables.

---
Feel free to submit pull requests and file bugs.

<center style="display: flex; gap: 1rem;">
Technology: Qt6

|

Language: C++

</center>
