use File::Basename;
use lib (dirname($0) . '/../perl/lib');

package BarnOwl;

sub bootstrap {}
sub get_data_dir {"."}
sub get_config_dir {"."}
sub create_style {}

our $ml = BarnOwl::MessageList->new();
sub message_list {$ml}

use BarnOwl;
