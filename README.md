# HMIS

Store patient records and generate a monthly HMIS 105 report for your medical facility.

![HMIS Screenshot](./hmis.png)

This software is meant to help you aggregate data for attendances and diagnoses only. 
It is not a complete hospital management system.

If you are interested in a complete hospital system, [send me an email](mailto:nabiira2by2@gmail.com).


## Installation:
1. Download the latest installer for windows from the releases page at https://github.com/abiiranathan/hmis/releases/tag/v1.0.0
   
2. Clone the repository and build you own installer.
   
See [QtInstaller Project](https://github.com/abiiranathan/qtinstaller) and learn how to create windows installers for Qt applications with ease.
---

## Known issues

HMIS uses a sqlite3 database. When running HMIS on windows, you may get an `Error opening database`. This is because the application needs read and write permissions on the C:\ drive.

To solve this issue, right click on the Desktop Icon -> Properties -> Compatibility tab -> Check Run this program as an Administrator. This enables HMIS to run as Administrator persistently. The longer option is to manually change permissions from the Security Tab under Properties.

<center style="display: flex; gap: 1rem;">
Technology: Qt6

|

Language: C++

</center>
