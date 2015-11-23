from registration.forms import RegistrationFormNoFreeEmail
from registration.users import UsernameField
from django.forms.fields import EmailField
from django.forms import HiddenInput
from freppledb.common.models import User 


class RegistrationfrePPLeCloud(RegistrationFormNoFreeEmail):
    
  class Meta:
      model = User
      widgets = {'username': HiddenInput()}
      fields = (UsernameField(), "email")
  
  def __init__(self, *args, **kwargs):
    super(RegistrationfrePPLeCloud, self).__init__(*args, **kwargs)
    self.fields['username'].required = False
    
  def clean_username(self):
    data = self.data['email']
    return data
  
