# Building P4Ruby from Source


1. Download the Perforce C++ API from the Perforce FTP site at
   <ftp://ftp.perforce.com/perforce>. \
   The API archive is located in release and platform-specific subdirectories and is named
   *"p4api-glibc2.3-openssl1.1.1.tgz".<br><br>*

   **Note: 32-bit builds of P4Ruby require a 32-bit version of the C++ API and a 32-bit version of Ruby.\
           64-bit builds of P4Ruby require a 64-bit version of the C++ API and a 64-bit version of Ruby.<br><br>**
   Unzip the archive into an empty directory.<br><br>

2. Extract the P4Ruby API archive into a new, empty directory.<br><br>

3. Execute the build commands:<br><br>

   *bundle install \
   bundle exec rake compile -- --with-p4api_dir=<absolute path to Perforce C++ API> \
              --with-ssl-dir=<absolute path to OpenSSL libraries matching Perforce C++ API><br><br>*
   
   OR pass through environment variables\
   *bundle exec rake compile p4api_dir=<*absolute path to Perforce C++ API*><br><br>*

   **Note: If the --p4api_dir flag is not provided, P4Ruby will attempt\
   to download and extract correct version of Perforce C++ API<br><br>**

4. Test your distribution.<br><br>

    *bundle exec rake test<br><br>*

    Tests require the perforce server binary (p4d) present in the path.<br><br>

5. Install P4Ruby into your local gem cache:<br><br>

    *bundle exec rake install*

## SSL support

Perforce Server 2012.1 and later supports SSL connections and the
C++ API has been compiled with this support.

If the P4Ruby build detects that OpenSSL is available, it will be
included by default. If you want to use libraries deployed to nonstandard
paths, use the --ssl_dir=<*path to Openssl include and lib folders*>