## Lazy update
- Q : number of words that are dominant (in terms of probability) so that those
      will be updated online (instead of offline), which are called in active set.

``model->tar`` would be updated in three ways:
- the positive part will be updated always online
- the negative part are separated into two groups:
    - for wrods in "active set", they will be updated online
    - for words not in "active set" (having small probability), weight are updated
    until offline computation occurs



