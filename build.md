# Building P4Ruby from Source


1. Make sure you have following packages installed on your system:\
        a. build-essential\
        b. libssl-dev

2. Download the Perforce C++ API from the Perforce FTP site at
   <https://ftp.perforce.com/perforce>. \
   The API archive is located in release and platform-specific subdirectories and is named
   *"p4api-glibc2.3-openssl1.1.1.tgz".*

   **Note:** 32-bit builds of P4Ruby require a 32-bit version of the C++ API and a 32-bit version of Ruby.\
           64-bit builds of P4Ruby require a 64-bit version of the C++ API and a 64-bit version of Ruby.
           
   Unzip the archive into an empty directory.

3. Extract the P4Ruby API archive into a new, empty directory.

4. Execute the build commands:

   ```
   bundle install
   bundle exec rake compile -- --with-p4api_dir=<absolute path to Perforce C++ API> --with-ssl-dir=<absolute path to OpenSSL libraries matching Perforce C++ API>
   ```
   OR pass through environment variables
   ```
   bundle exec rake compile p4api_dir=<absolute path to Perforce C++ API>
   ``` 
   **Note:** If the --p4api_dir flag is not provided, P4Ruby will attempt
   to download and extract correct version of Perforce C++ API.

5. Test your distribution.

    ```
    bundle exec rake test
    ```

    Tests require the perforce server binary (p4d) present in the path.
   
6. Install P4Ruby into your local gem cache:

    ```
    bundle exec rake gem
    gem install pkg/p4ruby*.gem -- --with-p4api_dir=<absolute path to Perforce C++ API>
    ```

## SSL support

Perforce Server 2012.1 and later supports SSL connections and the
C++ API has been compiled with this support.

If the P4Ruby build detects that OpenSSL is available, it will be
included by default. If you want to use libraries deployed to nonstandard
paths, use the --ssl_dir=<*path to Openssl include and lib folders*>
