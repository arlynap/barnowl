use strict;
use warnings;

package BarnOwl::MessageList;

sub binsearch {
    my $list = shift;
    my $val  = shift;
    my $key  = shift || sub {return $_[0]};
    my $left = 0;
    my $right = scalar @{$list} - 1;
    my $mid = $left;
    while($left < $right) {
        $mid = int(($left + $right)/2);
        my $k = $key->($list->[$mid]);
        if($k == $val) {
            return $mid;
        } elsif ($k < $val) {
            $left = $mid + 1;
        } else {
            $right = $mid - 1;
        }
    }
    return $left;
}

my $__next_id = 0;

sub next_id {
    return $__next_id++;
}

sub new {
    my $ml;
    eval q{
        use BarnOwl::MessageList::SQL;
        $ml = BarnOwl::MessageList::SQL->new;
    };
    
    if($@) {
        push @BarnOwl::__startup_errors, "Unable to load SQL message list\n$@";
    } else {
        return $ml;
    }
    my $class = shift;
    my $self = {messages => {}};
    return bless $self, $class;
}

sub set_attribute {
    
}

sub get_size {
    my $self = shift;
    return scalar keys %{$self->{messages}};
}

sub iterate_begin {
    my $self = shift;
    my $id   = shift;
    my $rev  = shift;
    $self->{keys} = [sort {$a <=> $b} keys %{$self->{messages}}];
    if($id < 0) {
        $self->{iterator} = scalar @{$self->{keys}} - 1;
    } else {
        $self->{iterator} = binsearch($self->{keys}, $id);
    }
    
    $self->{iterate_direction} = $rev ? -1 : 1;
}

sub iterate_next {
    my $self = shift;
    if($self->{iterator} >= scalar @{$self->{keys}}
       || $self->{iterator} < 0) {
        return undef;
    }

    my $msg = $self->get_by_id($self->{keys}->[$self->{iterator}]);
    $self->{iterator} += $self->{iterate_direction};
    return $msg;
}

sub iterate_done {
    # nop
}

sub get_by_id {
    my $self = shift;
    my $id = shift;
    return $self->{messages}{$id};
}

sub add_message {
    my $self = shift;
    my $m = shift;
    $self->{messages}->{$m->id} = $m;
}

sub expunge {
    my $self = shift;
    for my $message (values %{$self->{messages}}) {
        if($message->is_deleted) {
            delete $self->{messages}->{$message->id};
            BarnOwl::View->message_deleted($message->id);
        }
    }
}

sub close {
    
}


1;
